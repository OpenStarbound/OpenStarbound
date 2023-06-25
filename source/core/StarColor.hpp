#ifndef STAR_COLOR_HPP
#define STAR_COLOR_HPP

#include "StarString.hpp"
#include "StarVector.hpp"

namespace Star {

STAR_EXCEPTION(ColorException, StarException);

class Color {
public:
  static Color const Red;
  static Color const Orange;
  static Color const Yellow;
  static Color const Green;
  static Color const Blue;
  static Color const Indigo;
  static Color const Violet;
  static Color const Black;
  static Color const White;
  static Color const Magenta;
  static Color const DarkMagenta;
  static Color const Cyan;
  static Color const DarkCyan;
  static Color const CornFlowerBlue;
  static Color const Gray;
  static Color const LightGray;
  static Color const DarkGray;
  static Color const DarkGreen;
  static Color const Pink;
  static Color const Clear;

  static CaseInsensitiveStringMap<Color> const NamedColors;

  // Some useful conversion methods for dealing with Vec3 / Vec4 as colors
  static Vec3F v3bToFloat(Vec3B const& b);
  static Vec3B v3fToByte(Vec3F const& f, bool doClamp = true);
  static Vec4F v4bToFloat(Vec4B const& b);
  static Vec4B v4fToByte(Vec4F const& f, bool doClamp = true);

  static Color rgbf(float r, float g, float b);
  static Color rgbaf(float r, float g, float b, float a);
  static Color rgbf(Vec3F const& c);
  static Color rgbaf(Vec4F const& c);

  static Color rgb(uint8_t r, uint8_t g, uint8_t b);
  static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  static Color rgb(Vec3B const& c);
  static Color rgba(Vec4B const& c);

  static Color hsv(float h, float s, float b);
  static Color hsva(float h, float s, float b, float a);
  static Color hsv(Vec3F const& c);
  static Color hsva(Vec4F const& c);

  static Color grayf(float g);
  static Color gray(uint8_t g);

  // Only supports 8 bit color
  static Color fromHex(String const& s);

  // #AARRGGBB
  static Color fromUint32(uint32_t v);

  // Color from temperature in Kelvin
  static Color temperature(float temp);

  static Vec4B hueShiftVec4B(Vec4B color, float hue);
  static Vec4B Color::hexToVec4B(String const& s);
  // Black
  Color();

  explicit Color(String const& name);

  uint8_t red() const;
  uint8_t green() const;
  uint8_t blue() const;
  uint8_t alpha() const;

  void setRed(uint8_t r);
  void setGreen(uint8_t g);
  void setBlue(uint8_t b);
  void setAlpha(uint8_t a);

  float redF() const;
  float greenF() const;
  float blueF() const;
  float alphaF() const;

  void setRedF(float r);
  void setGreenF(float b);
  void setBlueF(float g);
  void setAlphaF(float a);

  bool isClear() const;

  // Returns a 4 byte value equal to #AARRGGBB
  uint32_t toUint32() const;

  Vec4B toRgba() const;
  Vec3B toRgb() const;
  Vec4F toRgbaF() const;
  Vec3F toRgbF() const;

  Vec4F toHsva() const;

  String toHex() const;

  float hue() const;
  float saturation() const;
  float value() const;

  void setHue(float hue);
  void setSaturation(float saturation);
  void setValue(float value);

  // Shift the current hue by the given value, with hue wrapping.
  void hueShift(float hue);

  // Reduce the color toward black by the given amount, from 0.0 to 1.0.
  void fade(float value);

  void convertToLinear();
  void convertToSRGB();

  Color toLinear();
  Color toSRGB();

  Color contrasting();
  Color complementary();

  // Mix two colors, giving the second color the given amount
  Color mix(Color const& c, float amount = 0.5f) const;
  Color multiply(float amount) const;

  bool operator==(Color const& c) const;
  bool operator!=(Color const& c) const;
  Color operator+(Color const& c) const;
  Color operator*(Color const& c) const;
  Color& operator+=(Color const& c);
  Color& operator*=(Color const& c);

  static float toLinear(float in);
  static float fromLinear(float in);
private:
  Vec4F m_data;
};

std::ostream& operator<<(std::ostream& os, Color const& c);

inline Vec3F Color::v3bToFloat(Vec3B const& b) {
  return Vec3F(byteToFloat(b[0]), byteToFloat(b[1]), byteToFloat(b[2]));
}

inline Vec3B Color::v3fToByte(Vec3F const& f, bool doClamp) {
  return Vec3B(floatToByte(f[0], doClamp), floatToByte(f[1], doClamp), floatToByte(f[2], doClamp));
}

inline Vec4F Color::v4bToFloat(Vec4B const& b) {
  return Vec4F(byteToFloat(b[0]), byteToFloat(b[1]), byteToFloat(b[2]), byteToFloat(b[3]));
}

inline Vec4B Color::v4fToByte(Vec4F const& f, bool doClamp) {
  return Vec4B(floatToByte(f[0], doClamp), floatToByte(f[1], doClamp), floatToByte(f[2], doClamp), floatToByte(f[3], doClamp));
}

}

#endif
