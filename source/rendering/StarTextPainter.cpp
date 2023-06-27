#include "StarTextPainter.hpp"
#include "StarJsonExtra.hpp"

#include <regex>

namespace Star {

namespace Text {
  static auto stripEscapeRegex = std::regex(strf("\\{:c}[^;]*{:c}", CmdEsc, EndEsc));
  String stripEscapeCodes(String const& s) {
    return std::regex_replace(s.utf8(), stripEscapeRegex, "");
  }

  String preprocessEscapeCodes(String const& s) {
    bool escape = false;
    auto result = s.utf8();

    size_t escapeStartIdx = 0;
    for (size_t i = 0; i < result.size(); i++) {
      auto& c = result[i];
      if (c == CmdEsc || c == StartEsc) {
        escape = true;
        escapeStartIdx = i;
      }
      if ((c <= SpecialCharLimit) && !(c == StartEsc))
        escape = false;
      if ((c == EndEsc) && escape)
        result[escapeStartIdx] = StartEsc;
    }
    return {result};
  }

  String extractCodes(String const& s) {
    bool escape = false;
    StringList result;
    String escapeCode;
    for (auto c : preprocessEscapeCodes(s)) {
      if (c == StartEsc)
        escape = true;
      if (c == EndEsc) {
        escape = false;
        for (auto command : escapeCode.split(','))
          result.append(command);
        escapeCode = "";
      }
      if (escape && (c != StartEsc))
        escapeCode.append(c);
    }
    if (!result.size())
      return "";
    return "^" + result.join(",") + ";";
  }
}

TextPositioning::TextPositioning() {
  pos = Vec2F();
  hAnchor = HorizontalAnchor::LeftAnchor;
  vAnchor = VerticalAnchor::BottomAnchor;
}

TextPositioning::TextPositioning(Vec2F pos, HorizontalAnchor hAnchor, VerticalAnchor vAnchor,
    Maybe<unsigned> wrapWidth, Maybe<unsigned> charLimit)
  : pos(pos), hAnchor(hAnchor), vAnchor(vAnchor), wrapWidth(wrapWidth), charLimit(charLimit) {}

TextPositioning::TextPositioning(Json const& v) {
  pos = v.opt("position").apply(jsonToVec2F).value();
  hAnchor = HorizontalAnchorNames.getLeft(v.getString("horizontalAnchor", "left"));
  vAnchor = VerticalAnchorNames.getLeft(v.getString("verticalAnchor", "top"));
  wrapWidth = v.optUInt("wrapWidth");
  charLimit = v.optUInt("charLimit");
}

Json TextPositioning::toJson() const {
  return JsonObject{
    {"position", jsonFromVec2F(pos)},
    {"horizontalAnchor", HorizontalAnchorNames.getRight(hAnchor)},
    {"verticalAnchor", VerticalAnchorNames.getRight(vAnchor)},
    {"wrapWidth", jsonFromMaybe(wrapWidth)}
  };
}

TextPositioning TextPositioning::translated(Vec2F translation) const {
  return {pos + translation, hAnchor, vAnchor, wrapWidth, charLimit};
}

TextPainter::TextPainter(RendererPtr renderer, TextureGroupPtr textureGroup)
  : m_renderer(renderer),
    m_fontTextureGroup(textureGroup),
    m_fontSize(8),
    m_lineSpacing(1.30f),
    m_renderSettings({FontMode::Normal, Vec4B::filled(255), "hobo", ""}),
    m_splitIgnore(" \t"),
    m_splitForce("\n\v"),
    m_nonRenderedCharacters("\n\v\r") {
  reloadFonts();
  m_reloadTracker = make_shared<TrackerListener>();
  Root::singleton().registerReloadListener(m_reloadTracker);
}

RectF TextPainter::renderText(String const& s, TextPositioning const& position) {
  if (position.charLimit) {
    unsigned charLimit = *position.charLimit;
    return doRenderText(s, position, true, &charLimit);
  } else {
    return doRenderText(s, position, true, nullptr);
  }
}

RectF TextPainter::renderLine(String const& s, TextPositioning const& position) {
  if (position.charLimit) {
    unsigned charLimit = *position.charLimit;
    return doRenderLine(s, position, true, &charLimit);
  } else {
    return doRenderLine(s, position, true, nullptr);
  }
}

RectF TextPainter::renderGlyph(String::Char c, TextPositioning const& position) {
  return doRenderGlyph(c, position, true);
}

RectF TextPainter::determineTextSize(String const& s, TextPositioning const& position) {
  return doRenderText(s, position, false, nullptr);
}

RectF TextPainter::determineLineSize(String const& s, TextPositioning const& position) {
  return doRenderLine(s, position, false, nullptr);
}

RectF TextPainter::determineGlyphSize(String::Char c, TextPositioning const& position) {
  return doRenderGlyph(c, position, false);
}

int TextPainter::glyphWidth(String::Char c) {
  return m_fontTextureGroup.glyphWidth(c, m_fontSize);
}

int TextPainter::stringWidth(String const& s) {
  String font = m_renderSettings.font, setFont = font;
  m_fontTextureGroup.switchFont(font);
  int width = 0;
  bool escape = false;

  String escapeCode;
  for (String::Char c : Text::preprocessEscapeCodes(s)) {
    if (c == Text::StartEsc)
      escape = true;

    if (!escape)
      width += glyphWidth(c);
    else if (c == Text::EndEsc) {
      auto commands = escapeCode.split(',');
      for (auto& command : commands) {
        if (command == "reset")
          m_fontTextureGroup.switchFont(font = setFont);
        else if (command == "set")
          setFont = font;
        else if (command.beginsWith("font="))
          m_fontTextureGroup.switchFont(font = command.substr(5));
      }
      escape = false;
      escapeCode = "";
    }
    if (escape && (c != Text::StartEsc))
      escapeCode.append(c);
  }

  return width;
}

StringList TextPainter::wrapText(String const& s, Maybe<unsigned> wrapWidth) {
  String font = m_renderSettings.font, setFont = font;
  m_fontTextureGroup.switchFont(font);
  String text = Text::preprocessEscapeCodes(s);

  unsigned lineStart = 0; // Where does this line start ?
  unsigned lineCharSize = 0; // how many characters in this line ?
  unsigned linePixelWidth = 0; // How wide is this line so far

  bool inEscapeSequence = false;
  String escapeCode;

  unsigned splitPos = 0; // Where did we last see a place to split the string ?
  unsigned splitWidth = 0; // How wide was the string there ?

  StringList lines; // list of renderable string lines

  // loop through every character in the string

  for (auto character : text) {
    // this up here to deal with the (common) occurance that the first charcter
    // is an escape initiator
    if (character == Text::StartEsc)
      inEscapeSequence = true;

    if (inEscapeSequence) {
      lineCharSize++; // just jump straight to the next character, we don't care what it is.
      if (character == Text::EndEsc) {
        auto commands = escapeCode.split(',');
        for (auto& command : commands) {
          if (command == "reset")
            m_fontTextureGroup.switchFont(font = setFont);
          else if (command == "set")
            setFont = font;
          else if (command.beginsWith("font="))
            m_fontTextureGroup.switchFont(font = command.substr(5));
        }
        inEscapeSequence = false;
        escapeCode = "";
      }
      if (character != Text::StartEsc)
        escapeCode.append(character);
    } else {
      lineCharSize++; // assume at least one character if we get here.

      // is this a linefeed / cr / whatever that forces a line split ?
      if (m_splitForce.find(String(character)) != NPos) {
        // knock one off the end because we don't render the CR
        lines.push_back(text.substr(lineStart, lineCharSize - 1));

        lineStart += lineCharSize; // next line starts after the CR
        lineCharSize = 0; // with no characters in it.
        linePixelWidth = 0; // No width
        splitPos = 0; // and no known splits.
      } else {
        int charWidth = glyphWidth(character);

        // is it a place where we might want to split the line ?
        if (m_splitIgnore.find(String(character)) != NPos) {
          splitPos = lineStart + lineCharSize; // this is the character after the space.
          splitWidth = linePixelWidth + charWidth; // the width of the string at
          // the split point, i.e. after the space.
        }

        // would the line be too long if we render this next character ?
        if (wrapWidth && (linePixelWidth + charWidth) > *wrapWidth) {
          // did we find somewhere to split the line ?
          if (splitPos) {
            lines.push_back(text.substr(lineStart, (splitPos - lineStart) - 1));

            unsigned stringEnd = lineStart + lineCharSize;
            lineCharSize = stringEnd - splitPos; // next line has the characters after the space.

            unsigned stringWidth = (linePixelWidth - splitWidth);
            linePixelWidth = stringWidth + charWidth; // and is as wide as the bit after the space.

            lineStart = splitPos; // next line starts after the space
            splitPos = 0; // split is used up.
          } else {
            // don't draw the last character that puts us over the edge
            lines.push_back(text.substr(lineStart, lineCharSize - 1));

            lineStart += lineCharSize - 1; // skip back by one to include that
            // character on the next line.
            lineCharSize = 1; // next line has that character in
            linePixelWidth = charWidth; // and is as wide as that character
          }
        } else {
          linePixelWidth += charWidth;
        }
      }
    }
  }

  // if we hit the end of the string before hitting the end of the line.
  if (lineCharSize > 0)
    lines.push_back(text.substr(lineStart, lineCharSize));

  return lines;
}

unsigned TextPainter::fontSize() const {
  return m_fontSize;
}

void TextPainter::setFontSize(unsigned size) {
  m_fontSize = size;
}

void TextPainter::setLineSpacing(float lineSpacing) {
  m_lineSpacing = lineSpacing;
}

void TextPainter::setMode(FontMode mode) {
  m_renderSettings.mode = mode;
}

void TextPainter::setSplitIgnore(String const& splitIgnore) {
  m_splitIgnore = splitIgnore;
}

void TextPainter::setFontColor(Vec4B color) {
  m_renderSettings.color = move(color);
}

void TextPainter::setProcessingDirectives(String directives) {
  m_renderSettings.directives = move(directives);
}

void TextPainter::setFont(String const& font) {
  m_renderSettings.font = font;
}

void TextPainter::addFont(FontPtr const& font, String const& name) {
  m_fontTextureGroup.addFont(font, name);
}

void TextPainter::reloadFonts() {
  m_fontTextureGroup.clearFonts();
  m_fontTextureGroup.cleanup(0);
  auto assets = Root::singleton().assets();
  auto defaultFont = assets->font("/hobo.ttf");
  for (auto& fontPath : assets->scanExtension("ttf")) {
    auto font = assets->font(fontPath);
    if (font == defaultFont)
      continue;

    auto fileName = AssetPath::filename(fontPath);
    addFont(font->clone(), fileName.substr(0, fileName.findLast(".")));
  }
  m_fontTextureGroup.addFont(defaultFont->clone(), "hobo", true);
}

void TextPainter::cleanup(int64_t timeout) {
  m_fontTextureGroup.cleanup(timeout);
}

void TextPainter::applyCommands(String const& unsplitCommands) {
  auto commands = unsplitCommands.split(',');
  for (auto& command : commands) {
    try {
      if (command == "reset") {
        m_renderSettings = m_savedRenderSettings;
      } else if (command == "set") {
        m_savedRenderSettings = m_renderSettings;
      } else if (command == "shadow") {
        m_renderSettings.mode = (FontMode)((int)m_renderSettings.mode | (int)FontMode::Shadow);
      } else if (command == "noshadow") {
        m_renderSettings.mode = (FontMode)((int)m_renderSettings.mode & (-1 ^ (int)FontMode::Shadow));
      } else if (command.beginsWith("font=")) {
        m_renderSettings.font = command.substr(5);
      } else if (command.beginsWith("directives=")) {
        // Honestly this is really stupid but I just couldn't help myself
        // Should probably limit in the future
        m_renderSettings.directives = command.substr(11);
      } else {
        // expects both #... sequences and plain old color names.
        Color c = jsonToColor(command);
        c.setAlphaF(c.alphaF() * ((float)m_savedRenderSettings.color[3]) / 255);
        m_renderSettings.color = c.toRgba();
      }
    } catch (JsonException&) {
    } catch (ColorException&) {
    }
  }
}

RectF TextPainter::doRenderText(String const& s, TextPositioning const& position, bool reallyRender, unsigned* charLimit) {
  Vec2F pos = position.pos;
  StringList lines = wrapText(s, position.wrapWidth);

  int height = (lines.size() - 1) * m_lineSpacing * m_fontSize + m_fontSize;

  RenderSettings backupRenderSettings = m_renderSettings;
  m_savedRenderSettings = m_renderSettings;

  if (position.vAnchor == VerticalAnchor::BottomAnchor)
    pos[1] += (height - m_fontSize);
  else if (position.vAnchor == VerticalAnchor::VMidAnchor)
    pos[1] += (height - m_fontSize) / 2;

  RectF bounds = RectF::withSize(pos, Vec2F());
  for (auto i : lines) {
    bounds.combine(doRenderLine(i, { pos, position.hAnchor, position.vAnchor }, reallyRender, charLimit));
    pos[1] -= m_fontSize * m_lineSpacing;

    if (charLimit && *charLimit == 0)
      break;
  }

  m_renderSettings = move(backupRenderSettings);

  return bounds;
}

RectF TextPainter::doRenderLine(String const& s, TextPositioning const& position, bool reallyRender, unsigned* charLimit) {
  if (m_reloadTracker->pullTriggered())
    reloadFonts();
  String text = s;
  TextPositioning pos = position;


  if (pos.hAnchor == HorizontalAnchor::RightAnchor) {
    auto trimmedString = s;
    if (charLimit)
      trimmedString = s.slice(0, *charLimit);
    pos.pos[0] -= stringWidth(trimmedString);
    pos.hAnchor = HorizontalAnchor::LeftAnchor;
  } else if (pos.hAnchor == HorizontalAnchor::HMidAnchor) {
    auto trimmedString = s;
    if (charLimit)
      trimmedString = s.slice(0, *charLimit);
    unsigned width = stringWidth(trimmedString);
    pos.pos[0] -= std::floor(width / 2);
    pos.hAnchor = HorizontalAnchor::LeftAnchor;
  }

  bool escape = false;
  String escapeCode;
  RectF bounds = RectF::withSize(pos.pos, Vec2F());
  for (String::Char c : text) {
    if (c == Text::StartEsc)
      escape = true;

    if (!escape) {
      if (charLimit) {
        if (*charLimit == 0)
          break;
        else
          --*charLimit;
      }
      RectF glyphBounds = doRenderGlyph(c, pos, reallyRender);
      bounds.combine(glyphBounds);
      pos.pos[0] += glyphBounds.width();
    } else if (c == Text::EndEsc) {
      applyCommands(escapeCode);
      escape = false;
      escapeCode = "";
    }
    if (escape && (c != Text::StartEsc))
      escapeCode.append(c);
  }

  return bounds;
}

RectF TextPainter::doRenderGlyph(String::Char c, TextPositioning const& position, bool reallyRender) {
  if (m_nonRenderedCharacters.find(String(c)) != NPos)
    return RectF();
  m_fontTextureGroup.switchFont(m_renderSettings.font);
  int width = glyphWidth(c);
  // Offset left by width if right anchored.
  float hOffset = 0;
  if (position.hAnchor == HorizontalAnchor::RightAnchor)
    hOffset = -width;
  else if (position.hAnchor == HorizontalAnchor::HMidAnchor)
    hOffset = -std::floor(width / 2);

  float vOffset = 0;
  if (position.vAnchor == VerticalAnchor::VMidAnchor)
    vOffset = -std::floor((float)m_fontSize / 2);
  else if (position.vAnchor == VerticalAnchor::TopAnchor)
    vOffset = -(float)m_fontSize;

  if (reallyRender) {
    if ((int)m_renderSettings.mode & (int)FontMode::Shadow) {
      Color shadow = Color::Black;
      uint8_t alphaU = m_renderSettings.color[3];
      if (alphaU != 255) {
        float alpha = byteToFloat(alphaU);
        shadow.setAlpha(floatToByte(alpha * (1.5f - 0.5f * alpha)));
      }
      else
        shadow.setAlpha(alphaU);

      //Kae: Draw only one shadow glyph instead of stacking two, alpha modified to appear perceptually the same as vanilla
      renderGlyph(c, position.pos + Vec2F(hOffset, vOffset - 2), m_fontSize, 1, shadow.toRgba(), m_renderSettings.directives);
    }

    renderGlyph(c, position.pos + Vec2F(hOffset, vOffset), m_fontSize, 1, m_renderSettings.color, m_renderSettings.directives);
  }

  return RectF::withSize(position.pos + Vec2F(hOffset, vOffset), {(float)width, (int)m_fontSize});
}

void TextPainter::renderGlyph(String::Char c, Vec2F const& screenPos, unsigned fontSize,
    float scale, Vec4B const& color, String const& processingDirectives) {
  if (!fontSize)
    return;

  const FontTextureGroup::GlyphTexture& glyphTexture = m_fontTextureGroup.glyphTexture(c, fontSize, processingDirectives);
  Vec2F offset = glyphTexture.processingOffset * (scale * 0.5f); //Kae: Re-center the glyph if the image scale was changed by the directives (it is positioned from the bottom left) 
  m_renderer->render(renderTexturedRect(glyphTexture.texture, Vec2F(screenPos) + offset, scale, color, 0.0f));
}

}
