#include "StarRenderer_opengl20.hpp"
#include "StarJsonExtra.hpp"
#include "StarCasting.hpp"
#include "StarLogging.hpp"

namespace Star {

size_t const MultiTextureCount = 4;

char const* DefaultVertexShader = R"SHADER(
#version 110

uniform vec2 textureSize0;
uniform vec2 textureSize1;
uniform vec2 textureSize2;
uniform vec2 textureSize3;
uniform vec2 screenSize;
uniform mat3 vertexTransform;

attribute vec2 vertexPosition;
attribute vec2 vertexTextureCoordinate;
attribute float vertexTextureIndex;
attribute vec4 vertexColor;
attribute float vertexParam1;

varying vec2 fragmentTextureCoordinate;
varying float fragmentTextureIndex;
varying vec4 fragmentColor;

void main() {
  vec2 screenPosition = (vertexTransform * vec3(vertexPosition, 1.0)).xy;
  gl_Position = vec4(screenPosition / screenSize * 2.0 - 1.0, 0.0, 1.0);
  if (vertexTextureIndex > 2.9) {
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize3;
  } else if (vertexTextureIndex > 1.9) {
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize2;
  } else if (vertexTextureIndex > 0.9) {
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize1;
  } else {
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize0;
  }
  fragmentTextureIndex = vertexTextureIndex;
  fragmentColor = vertexColor;
}
)SHADER";

char const* DefaultFragmentShader = R"SHADER(
#version 110

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;

varying vec2 fragmentTextureCoordinate;
varying float fragmentTextureIndex;
varying vec4 fragmentColor;

void main() {
  if (fragmentTextureIndex > 2.9) {
    gl_FragColor = texture2D(texture3, fragmentTextureCoordinate) * fragmentColor;
  } else if (fragmentTextureIndex > 1.9) {
    gl_FragColor = texture2D(texture2, fragmentTextureCoordinate) * fragmentColor;
  } else if (fragmentTextureIndex > 0.9) {
    gl_FragColor = texture2D(texture1, fragmentTextureCoordinate) * fragmentColor;
  } else {
    gl_FragColor = texture2D(texture0, fragmentTextureCoordinate) * fragmentColor;
  }
}
)SHADER";

OpenGl20Renderer::OpenGl20Renderer() {
  if (glewInit() != GLEW_OK)
    throw RendererException("Could not initialize GLEW");

  if (!GLEW_VERSION_2_0)
    throw RendererException("OpenGL 2.0 not available!");

  Logger::info("OpenGL version: '%s' vendor: '%s' renderer: '%s' shader: '%s'",
      glGetString(GL_VERSION),
      glGetString(GL_VENDOR),
      glGetString(GL_RENDERER),
      glGetString(GL_SHADING_LANGUAGE_VERSION));

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  m_whiteTexture = createGlTexture(Image::filled({1, 1}, Vec4B(255, 255, 255, 255), PixelFormat::RGBA32),
      TextureAddressing::Clamp,
      TextureFiltering::Nearest);
  m_immediateRenderBuffer = createGlRenderBuffer();

  setEffectConfig(JsonObject(), {{"vertex", DefaultVertexShader}, {"fragment", DefaultFragmentShader}});

  m_limitTextureGroupSize = false;
  m_useMultiTexturing = true;

  logGlErrorSummary("OpenGL errors during renderer initialization");
}

OpenGl20Renderer::~OpenGl20Renderer() {
  glDeleteProgram(m_program);

  logGlErrorSummary("OpenGL errors during shutdown");
}

String OpenGl20Renderer::rendererId() const {
  return "OpenGL20";
}

Vec2U OpenGl20Renderer::screenSize() const {
  return m_screenSize;
}

void OpenGl20Renderer::setEffectConfig(Json const& effectConfig, StringMap<String> const& shaders) {
  flushImmediatePrimitives();
  GLint status = 0;
  char logBuffer[1024];

  auto compileShader = [&](GLenum type, String const& name) -> GLuint {
    GLuint shader = glCreateShader(type);
    auto* source = shaders.ptr(name);
    if (!source)
      return 0;
    char const* sourcePtr = source->utf8Ptr();
    glShaderSource(shader, 1, &sourcePtr, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
      glGetShaderInfoLog(shader, sizeof(logBuffer), NULL, logBuffer);
      throw RendererException(strf("Failed to compile %s shader: %s\n", name, logBuffer));
    }

    return shader;
  };

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, "vertex");
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "fragment");

  GLuint program = glCreateProgram();

  if (vertexShader)
    glAttachShader(program, vertexShader);
  if (fragmentShader)
    glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  if (vertexShader)
    glDeleteShader(vertexShader);
  if (fragmentShader)
    glDeleteShader(fragmentShader);

  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (!status) {
    glGetProgramInfoLog(program, sizeof(logBuffer), NULL, logBuffer);
    glDeleteProgram(program);
    throw RendererException(strf("Failed to link program: %s\n", logBuffer));
  }

  if (m_program != 0)
    glDeleteProgram(m_program);
  m_program = program;
  glUseProgram(m_program);

  m_positionAttribute = glGetAttribLocation(m_program, "vertexPosition");
  m_texCoordAttribute = glGetAttribLocation(m_program, "vertexTextureCoordinate");
  m_texIndexAttribute = glGetAttribLocation(m_program, "vertexTextureIndex");
  m_colorAttribute = glGetAttribLocation(m_program, "vertexColor");
  m_param1Attribute = glGetAttribLocation(m_program, "vertexParam1");

  m_textureUniforms.clear();
  m_textureSizeUniforms.clear();
  for (size_t i = 0; i < MultiTextureCount; ++i) {
    m_textureUniforms.append(glGetUniformLocation(m_program, strf("texture%s", i).c_str()));
    m_textureSizeUniforms.append(glGetUniformLocation(m_program, strf("textureSize%s", i).c_str()));
  }
  m_screenSizeUniform = glGetUniformLocation(m_program, "screenSize");
  m_vertexTransformUniform = glGetUniformLocation(m_program, "vertexTransform");

  for (size_t i = 0; i < MultiTextureCount; ++i) {
    glUniform1i(m_textureUniforms[i], i);
  }
  glUniform2f(m_screenSizeUniform, m_screenSize[0], m_screenSize[1]);

  m_effectParameters.clear();

  for (auto const& p : effectConfig.getObject("effectParameters", {})) {
    EffectParameter effectParameter;

    effectParameter.parameterUniform = glGetUniformLocation(m_program, p.second.getString("uniform").utf8Ptr());
    if (effectParameter.parameterUniform == -1) {
      Logger::warn("OpenGL20 effect parameter '%s' has no associated uniform, skipping", p.first);
    } else {
      String type = p.second.getString("type");
      if (type == "bool") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<bool>();
      } else if (type == "int") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<int>();
      } else if (type == "float") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<float>();
      } else if (type == "vec2") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<Vec2F>();
      } else if (type == "vec3") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<Vec3F>();
      } else if (type == "vec4") {
        effectParameter.parameterType = RenderEffectParameter::typeIndexOf<Vec4F>();
      } else {
        throw RendererException::format("Unrecognized effect parameter type '%s'", type);
      }

      m_effectParameters[p.first] = effectParameter;

      if (Json def = p.second.get("default", {})) {
        if (type == "bool") {
          setEffectParameter(p.first, def.toBool());
        } else if (type == "int") {
          setEffectParameter(p.first, (int)def.toInt());
        } else if (type == "float") {
          setEffectParameter(p.first, def.toFloat());
        } else if (type == "vec2") {
          setEffectParameter(p.first, jsonToVec2F(def));
        } else if (type == "vec3") {
          setEffectParameter(p.first, jsonToVec3F(def));
        } else if (type == "vec4") {
          setEffectParameter(p.first, jsonToVec4F(def));
        }
      }
    }
  }

  m_effectTextures.clear();

  // Assign each texture parameter a texture unit starting with MultiTextureCount, the first
  // few texture units are used by the primary textures being drawn.  Currently,
  // maximum texture units are not checked.
  unsigned parameterTextureUnit = MultiTextureCount;

  for (auto const& p : effectConfig.getObject("effectTextures", {})) {
    EffectTexture effectTexture;
    effectTexture.textureUniform = glGetUniformLocation(m_program, p.second.getString("textureUniform").utf8Ptr());
    if (effectTexture.textureUniform == -1) {
      Logger::warn("OpenGL20 effect parameter '%s' has no associated uniform, skipping", p.first);
    } else {
        effectTexture.textureUnit = parameterTextureUnit++;
        glUniform1i(effectTexture.textureUniform, effectTexture.textureUnit);

        effectTexture.textureAddressing = TextureAddressingNames.getLeft(p.second.getString("textureAddressing", "clamp"));
        effectTexture.textureFiltering = TextureFilteringNames.getLeft(p.second.getString("textureFiltering", "nearest"));
        if (auto tsu = p.second.optString("textureSizeUniform")) {
          effectTexture.textureSizeUniform = glGetUniformLocation(m_program, tsu->utf8Ptr());
          if (effectTexture.textureSizeUniform == -1)
            Logger::warn("OpenGL20 effect parameter '%s' has textureSizeUniform '%s' with no associated uniform", p.first, *tsu);
        }

      m_effectTextures[p.first] = effectTexture;
    }
  }

  if (DebugEnabled)
    logGlErrorSummary("OpenGL errors setting effect config");
}

void OpenGl20Renderer::setEffectParameter(String const& parameterName, RenderEffectParameter const& value) {
  auto ptr = m_effectParameters.ptr(parameterName);
  if (!ptr || (ptr->parameterValue && *ptr->parameterValue == value))
    return;

  if (ptr->parameterType != value.typeIndex())
    throw RendererException::format("OpenGL20Renderer::setEffectParameter '%s' parameter type mismatch", parameterName);

  flushImmediatePrimitives();

  if (auto v = value.ptr<bool>())
    glUniform1i(ptr->parameterUniform, *v);
  else if (auto v = value.ptr<int>())
    glUniform1i(ptr->parameterUniform, *v);
  else if (auto v = value.ptr<float>())
    glUniform1f(ptr->parameterUniform, *v);
  else if (auto v = value.ptr<Vec2F>())
    glUniform2f(ptr->parameterUniform, (*v)[0], (*v)[1]);
  else if (auto v = value.ptr<Vec3F>())
    glUniform3f(ptr->parameterUniform, (*v)[0], (*v)[1], (*v)[2]);
  else if (auto v = value.ptr<Vec4F>())
    glUniform4f(ptr->parameterUniform, (*v)[0], (*v)[1], (*v)[2], (*v)[3]);

  ptr->parameterValue = value;
}

void OpenGl20Renderer::setEffectTexture(String const& textureName, Image const& image) {
  auto ptr = m_effectTextures.ptr(textureName);
  if (!ptr)
    return;

  flushImmediatePrimitives();

  if (!ptr->textureValue || ptr->textureValue->textureId == 0) {
    ptr->textureValue = createGlTexture(image, ptr->textureAddressing, ptr->textureFiltering);
  } else {
    glBindTexture(GL_TEXTURE_2D, ptr->textureValue->textureId);
    ptr->textureValue->textureSize = image.size();
    uploadTextureImage(image.pixelFormat(), image.size(), image.data());
  }

  if (ptr->textureSizeUniform != -1) {
    auto textureSize = ptr->textureValue->glTextureSize();
    glUniform2f(ptr->textureSizeUniform, textureSize[0], textureSize[1]);
  }
}

void OpenGl20Renderer::setScissorRect(Maybe<RectI> const& scissorRect) {
  if (scissorRect == m_scissorRect)
    return;

  flushImmediatePrimitives();

  m_scissorRect = scissorRect;
  if (m_scissorRect) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(m_scissorRect->xMin(), m_scissorRect->yMin(), m_scissorRect->width(), m_scissorRect->height());
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
}

TexturePtr OpenGl20Renderer::createTexture(Image const& texture, TextureAddressing addressing, TextureFiltering filtering) {
  return createGlTexture(texture, addressing, filtering);
}

void OpenGl20Renderer::setSizeLimitEnabled(bool enabled) {
  m_limitTextureGroupSize = enabled;
}

void OpenGl20Renderer::setMultiTexturingEnabled(bool enabled) {
  m_useMultiTexturing = enabled;
}

TextureGroupPtr OpenGl20Renderer::createTextureGroup(TextureGroupSize textureSize, TextureFiltering filtering) {
  int maxTextureSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

  // Large texture sizes are not always supported
  if (textureSize == TextureGroupSize::Large && (m_limitTextureGroupSize || maxTextureSize < 4096))
    textureSize = TextureGroupSize::Medium;

  unsigned atlasNumCells;
  if (textureSize == TextureGroupSize::Large)
    atlasNumCells = 256;
  else if (textureSize == TextureGroupSize::Medium)
    atlasNumCells = 128;
  else // TextureGroupSize::Small
    atlasNumCells = 64;

  Logger::info("detected supported OpenGL texture size %s, using atlasNumCells %s", maxTextureSize, atlasNumCells);

  auto glTextureGroup = make_shared<GlTextureGroup>(atlasNumCells);
  glTextureGroup->textureAtlasSet.textureFiltering = filtering;
  m_liveTextureGroups.append(glTextureGroup);
  return glTextureGroup;
}

RenderBufferPtr OpenGl20Renderer::createRenderBuffer() {
  return createGlRenderBuffer();
}

void OpenGl20Renderer::render(RenderPrimitive primitive) {
  m_immediatePrimitives.append(move(primitive));
}

void OpenGl20Renderer::renderBuffer(RenderBufferPtr const& renderBuffer, Mat3F const& transformation) {
  flushImmediatePrimitives();
  renderGlBuffer(*convert<GlRenderBuffer>(renderBuffer.get()), transformation);
}

void OpenGl20Renderer::flush() {
  flushImmediatePrimitives();
}

void OpenGl20Renderer::setScreenSize(Vec2U screenSize) {
  m_screenSize = screenSize;
  glViewport(0, 0, m_screenSize[0], m_screenSize[1]);
  glUniform2f(m_screenSizeUniform, m_screenSize[0], m_screenSize[1]);
}

void OpenGl20Renderer::startFrame() {
  if (m_scissorRect)
    glDisable(GL_SCISSOR_TEST);

  glClear(GL_COLOR_BUFFER_BIT);

  if (m_scissorRect)
    glEnable(GL_SCISSOR_TEST);
}

void OpenGl20Renderer::finishFrame() {
  flushImmediatePrimitives();
  // Make sure that the immediate render buffer doesn't needlessly lock texutres
  // from being compressed.
  m_immediateRenderBuffer->set({});

  filter(m_liveTextureGroups, [](auto const& p) {
        unsigned const CompressionsPerFrame = 1;

        if (!p.unique() || p->textureAtlasSet.totalTextures() > 0) {
          p->textureAtlasSet.compressionPass(CompressionsPerFrame);
          return true;
        }

        return false;
      });

  if (DebugEnabled)
    logGlErrorSummary("OpenGL errors this frame");
}

OpenGl20Renderer::GlTextureAtlasSet::GlTextureAtlasSet(unsigned atlasNumCells)
  : TextureAtlasSet(16, atlasNumCells) {}

GLuint OpenGl20Renderer::GlTextureAtlasSet::createAtlasTexture(Vec2U const& size, PixelFormat pixelFormat) {
  GLuint glTextureId;
  glGenTextures(1, &glTextureId);
  if (glTextureId == 0)
    throw RendererException("Could not generate texture in OpenGL20Renderer::TextureGroup::createAtlasTexture()");

  glBindTexture(GL_TEXTURE_2D, glTextureId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (textureFiltering == TextureFiltering::Nearest) {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  } else {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  uploadTextureImage(pixelFormat, size, nullptr);
  return glTextureId;
}

void OpenGl20Renderer::GlTextureAtlasSet::destroyAtlasTexture(GLuint const& glTexture) {
  glDeleteTextures(1, &glTexture);
}

void OpenGl20Renderer::GlTextureAtlasSet::copyAtlasPixels(
    GLuint const& glTexture, Vec2U const& bottomLeft, Image const& image) {
  glBindTexture(GL_TEXTURE_2D, glTexture);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (image.pixelFormat() == PixelFormat::RGB24) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, bottomLeft[0], bottomLeft[1], image.width(), image.height(), GL_RGB, GL_UNSIGNED_BYTE, image.data());
  } else if (image.pixelFormat() == PixelFormat::RGBA32) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, bottomLeft[0], bottomLeft[1], image.width(), image.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.data());
  } else if (image.pixelFormat() == PixelFormat::BGR24) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, bottomLeft[0], bottomLeft[1], image.width(), image.height(), GL_BGR, GL_UNSIGNED_BYTE, image.data());
  } else if (image.pixelFormat() == PixelFormat::BGRA32) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, bottomLeft[0], bottomLeft[1], image.width(), image.height(), GL_BGRA, GL_UNSIGNED_BYTE, image.data());
  } else {
    throw RendererException("Unsupported texture format in OpenGL20Renderer::TextureGroup::copyAtlasPixels");
  }
}

OpenGl20Renderer::GlTextureGroup::GlTextureGroup(unsigned atlasNumCells)
  : textureAtlasSet(atlasNumCells) {}

OpenGl20Renderer::GlTextureGroup::~GlTextureGroup() {
  textureAtlasSet.reset();
}

TextureFiltering OpenGl20Renderer::GlTextureGroup::filtering() const {
  return textureAtlasSet.textureFiltering;
}

TexturePtr OpenGl20Renderer::GlTextureGroup::create(Image const& texture) {
  // If the image is empty, or would not fit in the texture atlas with border
  // pixels, just create a regular texture
  Vec2U atlasTextureSize = textureAtlasSet.atlasTextureSize();
  if (texture.empty() || texture.width() + 2 > atlasTextureSize[0] || texture.height() + 2 > atlasTextureSize[1])
    return createGlTexture(texture, TextureAddressing::Clamp, textureAtlasSet.textureFiltering);

  auto glGroupedTexture = make_ref<GlGroupedTexture>();
  glGroupedTexture->parentGroup = shared_from_this();
  glGroupedTexture->parentAtlasTexture = textureAtlasSet.addTexture(texture);

  return glGroupedTexture;
}

OpenGl20Renderer::GlGroupedTexture::~GlGroupedTexture() {
  if (parentAtlasTexture)
    parentGroup->textureAtlasSet.freeTexture(parentAtlasTexture);
}

Vec2U OpenGl20Renderer::GlGroupedTexture::size() const {
  return parentAtlasTexture->imageSize();
}

TextureFiltering OpenGl20Renderer::GlGroupedTexture::filtering() const {
  return parentGroup->filtering();
}

TextureAddressing OpenGl20Renderer::GlGroupedTexture::addressing() const {
  return TextureAddressing::Clamp;
}

GLuint OpenGl20Renderer::GlGroupedTexture::glTextureId() const {
  return parentAtlasTexture->atlasTexture();
}

Vec2U OpenGl20Renderer::GlGroupedTexture::glTextureSize() const {
  return parentGroup->textureAtlasSet.atlasTextureSize();
}

Vec2U OpenGl20Renderer::GlGroupedTexture::glTextureCoordinateOffset() const {
  return parentAtlasTexture->atlasTextureCoordinates().min();
}

void OpenGl20Renderer::GlGroupedTexture::incrementBufferUseCount() {
  if (bufferUseCount == 0)
    parentAtlasTexture->setLocked(true);
  ++bufferUseCount;
}

void OpenGl20Renderer::GlGroupedTexture::decrementBufferUseCount() {
  starAssert(bufferUseCount != 0);
  if (bufferUseCount == 1)
    parentAtlasTexture->setLocked(false);
  --bufferUseCount;
}

OpenGl20Renderer::GlLoneTexture::~GlLoneTexture() {
  if (textureId != 0)
    glDeleteTextures(1, &textureId);
}

Vec2U OpenGl20Renderer::GlLoneTexture::size() const {
  return textureSize;
}

TextureFiltering OpenGl20Renderer::GlLoneTexture::filtering() const {
  return textureFiltering;
}

TextureAddressing OpenGl20Renderer::GlLoneTexture::addressing() const {
  return textureAddressing;
}

GLuint OpenGl20Renderer::GlLoneTexture::glTextureId() const {
  return textureId;
}

Vec2U OpenGl20Renderer::GlLoneTexture::glTextureSize() const {
  return textureSize;
}

Vec2U OpenGl20Renderer::GlLoneTexture::glTextureCoordinateOffset() const {
  return Vec2U();
}

OpenGl20Renderer::GlRenderBuffer::~GlRenderBuffer() {
  for (auto const& texture : usedTextures) {
    if (auto gt = as<GlGroupedTexture>(texture.get()))
      gt->decrementBufferUseCount();
  }
  for (auto const& vb : vertexBuffers)
    glDeleteBuffers(1, &vb.vertexBuffer);
}

void OpenGl20Renderer::GlRenderBuffer::set(List<RenderPrimitive> primitives) {
  for (auto const& texture : usedTextures) {
    if (auto gt = as<GlGroupedTexture>(texture.get()))
      gt->decrementBufferUseCount();
  }
  usedTextures.clear();

  auto oldVertexBuffers = take(vertexBuffers);

  List<GLuint> currentTextures;
  List<Vec2U> currentTextureSizes;
  size_t currentVertexCount = 0;

  auto finishCurrentBuffer = [&]() {
    if (currentVertexCount > 0) {
      GlVertexBuffer vb;
      for (size_t i = 0; i < currentTextures.size(); ++i) {
        vb.textures.append(GlVertexBufferTexture{currentTextures[i], currentTextureSizes[i]});
      }
      vb.vertexCount = currentVertexCount;
      if (!oldVertexBuffers.empty()) {
        auto oldVb = oldVertexBuffers.takeLast();
        vb.vertexBuffer = oldVb.vertexBuffer;
        glBindBuffer(GL_ARRAY_BUFFER, vb.vertexBuffer);
        if (oldVb.vertexCount >= vb.vertexCount)
          glBufferSubData(GL_ARRAY_BUFFER, 0, accumulationBuffer.size(), accumulationBuffer.ptr());
        else
          glBufferData(GL_ARRAY_BUFFER, accumulationBuffer.size(), accumulationBuffer.ptr(), GL_STREAM_DRAW);
      } else {
        glGenBuffers(1, &vb.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vb.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, accumulationBuffer.size(), accumulationBuffer.ptr(), GL_STREAM_DRAW);
      }

      vertexBuffers.append(vb);

      currentTextures.clear();
      currentTextureSizes.clear();
      accumulationBuffer.clear();
      currentVertexCount = 0;
    }
  };

  auto textureCount = useMultiTexturing ? MultiTextureCount : 1;
  auto addCurrentTexture = [&](TexturePtr texture) -> pair<uint8_t, Vec2F> {
    if (!texture)
      texture = whiteTexture;

    auto glTexture = as<GlTexture>(texture.get());
    GLuint glTextureId = glTexture->glTextureId();

    auto textureIndex = currentTextures.indexOf(glTextureId);
    if (textureIndex == NPos) {
      if (currentTextures.size() >= textureCount)
        finishCurrentBuffer();

      textureIndex = currentTextures.size();
      currentTextures.append(glTextureId);
      currentTextureSizes.append(glTexture->glTextureSize());
    }

    if (auto gt = as<GlGroupedTexture>(texture.get()))
      gt->incrementBufferUseCount();
    usedTextures.add(move(texture));

    return {float(textureIndex), Vec2F(glTexture->glTextureCoordinateOffset())};
  };

  auto appendBufferVertex = [&](RenderVertex v, float textureIndex, Vec2F textureCoordinateOffset) {
    GlRenderVertex glv {
      v.screenCoordinate,
      v.textureCoordinate + textureCoordinateOffset,
      textureIndex,
      v.color,
      v.param1
    };
    accumulationBuffer.append((char const*)&glv, sizeof(GlRenderVertex));
    ++currentVertexCount;
  };

  for (auto& primitive : primitives) {
    float textureIndex;
    Vec2F textureOffset;
    if (auto tri = primitive.ptr<RenderTriangle>()) {
      tie(textureIndex, textureOffset) = addCurrentTexture(move(tri->texture));

      appendBufferVertex(tri->a, textureIndex, textureOffset);
      appendBufferVertex(tri->b, textureIndex, textureOffset);
      appendBufferVertex(tri->c, textureIndex, textureOffset);

    } else if (auto quad = primitive.ptr<RenderQuad>()) {
      tie(textureIndex, textureOffset) = addCurrentTexture(move(quad->texture));

      appendBufferVertex(quad->a, textureIndex, textureOffset);
      appendBufferVertex(quad->b, textureIndex, textureOffset);
      appendBufferVertex(quad->c, textureIndex, textureOffset);

      appendBufferVertex(quad->a, textureIndex, textureOffset);
      appendBufferVertex(quad->c, textureIndex, textureOffset);
      appendBufferVertex(quad->d, textureIndex, textureOffset);

    } else if (auto poly = primitive.ptr<RenderPoly>()) {
      if (poly->vertexes.size() > 2) {
        tie(textureIndex, textureOffset) = addCurrentTexture(move(poly->texture));
        for (size_t i = 1; i < poly->vertexes.size() - 1; ++i) {
          appendBufferVertex(poly->vertexes[0], textureIndex, textureOffset);
          appendBufferVertex(poly->vertexes[i], textureIndex, textureOffset);
          appendBufferVertex(poly->vertexes[i + 1], textureIndex, textureOffset);
        }
      }
    }
  }

  finishCurrentBuffer();

  for (auto const& vb : oldVertexBuffers)
    glDeleteBuffers(1, &vb.vertexBuffer);
}

void OpenGl20Renderer::logGlErrorSummary(String prefix) {
  if (GLenum error = glGetError()) {
    prefix += ": ";
    Logger::error(prefix.utf8Ptr());
    do {
      if (error == GL_INVALID_ENUM) {
        Logger::error("GL_INVALID_ENUM");
      } else if (error == GL_INVALID_VALUE) {
        Logger::error("GL_INVALID_VALUE");
      } else if (error == GL_INVALID_OPERATION) {
        Logger::error("GL_INVALID_OPERATION");
      } else if (error == GL_INVALID_FRAMEBUFFER_OPERATION) {
        Logger::error("GL_INVALID_FRAMEBUFFER_OPERATION");
      } else if (error == GL_OUT_OF_MEMORY) {
        Logger::error("GL_OUT_OF_MEMORY");
      } else if (error == GL_STACK_UNDERFLOW) {
        Logger::error("GL_STACK_UNDERFLOW");
      } else if (error == GL_STACK_OVERFLOW) {
        Logger::error("GL_STACK_OVERFLOW");
      } else {
        Logger::error("<UNRECOGNIZED GL ERROR>");
      }
    } while ((error = glGetError()));
  }
}

void OpenGl20Renderer::uploadTextureImage(PixelFormat pixelFormat, Vec2U size, uint8_t const* data) {
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (pixelFormat == PixelFormat::RGB24) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size[0], size[1], 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  } else if (pixelFormat == PixelFormat::RGBA32) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  } else if (pixelFormat == PixelFormat::BGR24) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, size[0], size[1], 0, GL_BGR, GL_UNSIGNED_BYTE, data);
  } else if (pixelFormat == PixelFormat::BGRA32) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  } else {
    starAssert(false);
  }
}

void OpenGl20Renderer::flushImmediatePrimitives() {
  if (m_immediatePrimitives.empty())
    return;

  m_immediateRenderBuffer->set(take(m_immediatePrimitives));
  renderGlBuffer(*m_immediateRenderBuffer, Mat3F::identity());
}

auto OpenGl20Renderer::createGlTexture(Image const& image, TextureAddressing addressing, TextureFiltering filtering)
    -> RefPtr<GlLoneTexture> {
  auto glLoneTexture = make_ref<GlLoneTexture>();
  glLoneTexture->textureFiltering = filtering;
  glLoneTexture->textureAddressing = addressing;
  glLoneTexture->textureSize = image.size();

  if (image.empty())
    return glLoneTexture;

  glGenTextures(1, &glLoneTexture->textureId);
  if (glLoneTexture->textureId == 0)
    throw RendererException("Could not generate texture in OpenGL20Renderer::createGlTexture");

  glBindTexture(GL_TEXTURE_2D, glLoneTexture->textureId);

  if (addressing == TextureAddressing::Clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  if (filtering == TextureFiltering::Nearest) {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  } else {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  uploadTextureImage(image.pixelFormat(), image.size(), image.data());

  return glLoneTexture;
}

auto OpenGl20Renderer::createGlRenderBuffer() -> shared_ptr<GlRenderBuffer> {
  auto glrb = make_shared<GlRenderBuffer>();
  glrb->whiteTexture = m_whiteTexture;
  glrb->useMultiTexturing = m_useMultiTexturing;
  return glrb;
}

void OpenGl20Renderer::renderGlBuffer(GlRenderBuffer const& renderBuffer, Mat3F const& transformation) {
  for (auto const& vb : renderBuffer.vertexBuffers) {
    glUniformMatrix3fv(m_vertexTransformUniform, 1, GL_TRUE, transformation.ptr());

    for (size_t i = 0; i < vb.textures.size(); ++i) {
      glUniform2f(m_textureSizeUniforms[i], vb.textures[i].size[0], vb.textures[i].size[1]);
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, vb.textures[i].texture);
    }

    for (auto const& p : m_effectTextures) {
      if (p.second.textureValue) {
        glActiveTexture(GL_TEXTURE0 + p.second.textureUnit);
        glBindTexture(GL_TEXTURE_2D, p.second.textureValue->textureId);
      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vb.vertexBuffer);

    glEnableVertexAttribArray(m_positionAttribute);
    glEnableVertexAttribArray(m_texCoordAttribute);
    glEnableVertexAttribArray(m_texIndexAttribute);
    glEnableVertexAttribArray(m_colorAttribute);
    glEnableVertexAttribArray(m_param1Attribute);

    glVertexAttribPointer(m_positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GlRenderVertex), (GLvoid*)offsetof(GlRenderVertex, screenCoordinate));
    glVertexAttribPointer(m_texCoordAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GlRenderVertex), (GLvoid*)offsetof(GlRenderVertex, textureCoordinate));
    glVertexAttribPointer(m_texIndexAttribute, 1, GL_FLOAT, GL_FALSE, sizeof(GlRenderVertex), (GLvoid*)offsetof(GlRenderVertex, textureIndex));
    glVertexAttribPointer(m_colorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlRenderVertex), (GLvoid*)offsetof(GlRenderVertex, color));
    glVertexAttribPointer(m_param1Attribute, 1, GL_FLOAT, GL_FALSE, sizeof(GlRenderVertex), (GLvoid*)offsetof(GlRenderVertex, param1));
    
    glDrawArrays(GL_TRIANGLES, 0, vb.vertexCount);
  }
}

}
