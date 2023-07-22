#include "StarTextPainter.hpp"
#include "StarJsonExtra.hpp"
#include "StarText.hpp"

#include <regex>

namespace Star {

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
    m_renderSettings({FontMode::Normal, Vec4B::filled(255), "hobo", ""}) {
  reloadFonts();
  m_reloadTracker = make_shared<TrackerListener>();
  Root::singleton().registerReloadListener(m_reloadTracker);
}

RectF TextPainter::renderText(StringView s, TextPositioning const& position) {
  if (position.charLimit) {
    unsigned charLimit = *position.charLimit;
    return doRenderText(s, position, true, &charLimit);
  } else {
    return doRenderText(s, position, true, nullptr);
  }
}

RectF TextPainter::renderLine(StringView s, TextPositioning const& position) {
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

RectF TextPainter::determineTextSize(StringView s, TextPositioning const& position) {
  return doRenderText(s, position, false, nullptr);
}

RectF TextPainter::determineLineSize(StringView s, TextPositioning const& position) {
  return doRenderLine(s, position, false, nullptr);
}

RectF TextPainter::determineGlyphSize(String::Char c, TextPositioning const& position) {
  return doRenderGlyph(c, position, false);
}

int TextPainter::glyphWidth(String::Char c) {
  return m_fontTextureGroup.glyphWidth(c, m_fontSize);
}

int TextPainter::stringWidth(StringView s) {
  if (s.empty())
    return 0;

  String font = m_renderSettings.font, setFont = font;
  m_fontTextureGroup.switchFont(font);

  int width = 0;

  Text::CommandsCallback commandsCallback = [&](StringView commands) {
    commands.forEachSplitView(",", [&](StringView command, size_t, size_t) {
      if (command == "reset")
        m_fontTextureGroup.switchFont(font = setFont);
      else if (command == "set")
        setFont = font;
      else if (command.beginsWith("font="))
        m_fontTextureGroup.switchFont(font = command.substr(5));
    });
    return true;
  };

  Text::TextCallback textCallback = [&](StringView text) {
    for (String::Char c : text)
      width += glyphWidth(c);

    return true;
  };

  Text::processText(s, textCallback, commandsCallback);

  return width;
}

bool TextPainter::processWrapText(StringView text, unsigned* wrapWidth, WrapTextCallback textFunc) {
  String font = m_renderSettings.font, setFont = font;
  m_fontTextureGroup.switchFont(font);
  int lines = 0;

  size_t i = 0;
  auto it = text.begin();
  auto end = text.end();

  unsigned lineStart = 0; // Where does this line start ?
  unsigned linePixelWidth = 0; // How wide is this line so far
  unsigned lineCharSize = 0; // how many characters in this line ?

  auto escIt = end;

  unsigned splitPos = 0; // Where did we last see a place to split the string ?
  unsigned splitWidth = 0; // How wide was the string there ?

  auto lineStartIt = it; // Where does this line start ?
  auto splitIt = end;

  auto slice = [](StringView::const_iterator a, StringView::const_iterator b) -> StringView {
    const char* aPtr = &*a.base();
    return StringView(aPtr, &*b.base() - aPtr);
  };

  while (it != end) {
    auto character = *it;

    if (Text::isEscapeCode(character))
      escIt = it;
    ++i;
   

    if (escIt != end) {
      if (character == Text::EndEsc) {
        StringView inner = slice(escIt, it);
        inner.forEachSplitView(",", [&](StringView command, size_t, size_t) {
          if (command == "reset")
            m_fontTextureGroup.switchFont(font = setFont);
          else if (command == "set")
            setFont = font;
          else if (command.beginsWith("font="))
            m_fontTextureGroup.switchFont(font = command.substr(5));
        });
        escIt = end;
      }
      lineCharSize++;
    } else {
      lineCharSize++; // assume at least one character if we get here.

      // is this a linefeed / cr / whatever that forces a line split ?
      if (character == '\n' || character == '\v') {
        // knock one off the end because we don't render the CR
        if (!textFunc(slice(lineStartIt, it), lines++))
          return false;

        lineStart += lineCharSize; 
        lineStartIt = it;
        ++lineStartIt;

        lineCharSize = linePixelWidth = splitPos = 0; // ...with no characters in it and no known splits.
      } else {
        int charWidth = glyphWidth(character);

        // is it a place where we might want to split the line ?
        if (character == ' ' || character == '\t') {
          splitPos = lineStart + lineCharSize; // this is the character after the space.
          splitWidth = linePixelWidth + charWidth; // the width of the string at
          splitIt = it;
          ++splitIt;
          // the split point, i.e. after the space.
        }

        // would the line be too long if we render this next character ?
        if (wrapWidth && (linePixelWidth + charWidth) > *wrapWidth) {
          // did we find somewhere to split the line ?
          if (splitPos) {
            if (!textFunc(slice(lineStartIt, splitIt), lines++))
              return false;

            unsigned stringEnd = lineStart + lineCharSize;
            lineCharSize = stringEnd - splitPos; // next line has the characters after the space.

            unsigned stringWidth = (linePixelWidth - splitWidth);
            linePixelWidth = stringWidth + charWidth; // and is as wide as the bit after the space.

            lineStart = splitPos;
            lineStartIt = splitIt;

            splitPos = 0;
          } else {
            if (!textFunc(slice(lineStartIt, it), lines++))
              return false;

            lineStart += lineCharSize - 1;
            lineStartIt = it; // include that character on the next line.
            

            lineCharSize = 1;           // next line has that character in
            linePixelWidth = charWidth; // and is as wide as that character
          }
        } else {
          linePixelWidth += charWidth;
        }
      }
    }

    ++it;
  };

  // if we hit the end of the string before hitting the end of the line.
  if (lineCharSize > 0)
    return textFunc(slice(lineStartIt, end), lines);

  return true;
}

List<StringView> TextPainter::wrapTextViews(StringView s, Maybe<unsigned> wrapWidth) {
  List<StringView> views = {};

  bool active = false;
  StringView current;
  int lastLine = 0;

  auto addText = [&active, &current](StringView text) {
    // Merge views if they are adjacent
    if (active && current.utf8Ptr() + current.utf8Size() == text.utf8Ptr())
      current = StringView(current.utf8Ptr(), current.utf8Size() + text.utf8Size());
    else
      current = text;
    active = true;
  };

  TextPainter::WrapTextCallback textCallback = [&](StringView text, int line) {
    if (lastLine != line) {
      views.push_back(current);
      lastLine = line;
      active = false;
    }

    addText(text);
    return true;
  };

  processWrapText(s, wrapWidth.ptr(), textCallback);

  if (active)
    views.push_back(current);

  return views;
}

StringList TextPainter::wrapText(StringView s, Maybe<unsigned> wrapWidth) {
  StringList result;

  String current;
  int lastLine = 0;
  TextPainter::WrapTextCallback textCallback = [&](StringView text, int line) {
    if (lastLine != line) {
      result.append(move(current));
      lastLine = line;
    }

    current += text;
    return true;
  };

  processWrapText(s, wrapWidth.ptr(), textCallback);

  if (!current.empty())
    result.append(move(current));

  return result;
};

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

void TextPainter::setFontColor(Vec4B color) {
  m_renderSettings.color = move(color);
}

void TextPainter::setProcessingDirectives(StringView directives) {
  m_renderSettings.directives = String(directives);
  if (m_renderSettings.directives) {
    m_renderSettings.directives.loadOperations();
    for (auto& entry : m_renderSettings.directives.shared->entries) {
      if (auto border = entry.operation.ptr<BorderImageOperation>())
        border->includeTransparent = true;
    }
  }
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

void TextPainter::applyCommands(StringView unsplitCommands) {
  unsplitCommands.forEachSplitView(",", [&](StringView command, size_t, size_t) {
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
        setProcessingDirectives(command.substr(11));
      } else {
        // expects both #... sequences and plain old color names.
        Color c = Color(command);
        c.setAlphaF(c.alphaF() * ((float)m_savedRenderSettings.color[3]) / 255);
        m_renderSettings.color = c.toRgba();
      }
    } catch (JsonException&) {
    } catch (ColorException&) {
    }
  });
}

RectF TextPainter::doRenderText(StringView s, TextPositioning const& position, bool reallyRender, unsigned* charLimit) {
  Vec2F pos = position.pos;
  if (s.empty())
    return RectF(pos, pos);

  List<StringView> lines = wrapTextViews(s, position.wrapWidth);

  int height = (lines.size() - 1) * m_lineSpacing * m_fontSize + m_fontSize;

  RenderSettings backupRenderSettings = m_renderSettings;
  m_savedRenderSettings = m_renderSettings;

  if (position.vAnchor == VerticalAnchor::BottomAnchor)
    pos[1] += (height - m_fontSize);
  else if (position.vAnchor == VerticalAnchor::VMidAnchor)
    pos[1] += (height - m_fontSize) / 2;

  RectF bounds = RectF::withSize(pos, Vec2F());
  for (auto& i : lines) {
    bounds.combine(doRenderLine(i, { pos, position.hAnchor, position.vAnchor }, reallyRender, charLimit));
    pos[1] -= m_fontSize * m_lineSpacing;

    if (charLimit && *charLimit == 0)
      break;
  }

  m_renderSettings = move(backupRenderSettings);

  return bounds;
}

RectF TextPainter::doRenderLine(StringView text, TextPositioning const& position, bool reallyRender, unsigned* charLimit) {
  if (m_reloadTracker->pullTriggered())
    reloadFonts();
  TextPositioning pos = position;


  if (pos.hAnchor == HorizontalAnchor::RightAnchor) {
    StringView trimmedString = charLimit ? text.substr(0, *charLimit) : text;
    pos.pos[0] -= stringWidth(trimmedString);
    pos.hAnchor = HorizontalAnchor::LeftAnchor;
  } else if (pos.hAnchor == HorizontalAnchor::HMidAnchor) {
    StringView trimmedString = charLimit ? text.substr(0, *charLimit) : text;
    pos.pos[0] -= floor((float)stringWidth(trimmedString) / 2);
    pos.hAnchor = HorizontalAnchor::LeftAnchor;
  }

  bool escape = false;
  String escapeCode;
  RectF bounds = RectF::withSize(pos.pos, Vec2F());
  Text::TextCallback textCallback = [&](StringView text) {
    for (String::Char c : text) {
      if (charLimit) {
        if (*charLimit == 0)
          return false;
        else
          --* charLimit;
      }

      RectF glyphBounds = doRenderGlyph(c, pos, reallyRender);
      bounds.combine(glyphBounds);
      pos.pos[0] += glyphBounds.width();
    }
    return true;
  };

  Text::CommandsCallback commandsCallback = [&](StringView commands) {
    applyCommands(commands);
    return true;
  };

  m_fontTextureGroup.switchFont(m_renderSettings.font);
  Text::processText(text, textCallback, commandsCallback);

  return bounds;
}

RectF TextPainter::doRenderGlyph(String::Char c, TextPositioning const& position, bool reallyRender) {
  if (c == '\n' || c == '\v' || c == '\r')
    return RectF();
  m_fontTextureGroup.switchFont(m_renderSettings.font);

  int width = glyphWidth(c);
  // Offset left by width if right anchored.
  float hOffset = 0;
  if (position.hAnchor == HorizontalAnchor::RightAnchor)
    hOffset = -width;
  else if (position.hAnchor == HorizontalAnchor::HMidAnchor)
    hOffset = -floor((float)width / 2);

  float vOffset = 0;
  if (position.vAnchor == VerticalAnchor::VMidAnchor)
    vOffset = -floor((float)m_fontSize / 2);
  else if (position.vAnchor == VerticalAnchor::TopAnchor)
    vOffset = -(float)m_fontSize;

  Directives* directives = m_renderSettings.directives ? &m_renderSettings.directives : nullptr;

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
      renderGlyph(c, position.pos + Vec2F(hOffset, vOffset - 2), m_fontSize, 1, shadow.toRgba(), directives);
    }

    renderGlyph(c, position.pos + Vec2F(hOffset, vOffset), m_fontSize, 1, m_renderSettings.color, directives);
  }

  return RectF::withSize(position.pos + Vec2F(hOffset, vOffset), {(float)width, (int)m_fontSize});
}

void TextPainter::renderGlyph(String::Char c, Vec2F const& screenPos, unsigned fontSize,
    float scale, Vec4B const& color, Directives const* processingDirectives) {
  if (!fontSize)
    return;

  const FontTextureGroup::GlyphTexture& glyphTexture = m_fontTextureGroup.glyphTexture(c, fontSize, processingDirectives);
  Vec2F offset = glyphTexture.offset * scale;
  m_renderer->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), glyphTexture.texture, Vec2F::round(screenPos + offset), scale, color, 0.0f);
}

}
