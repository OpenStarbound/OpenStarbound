#include "StarImage.hpp"
#include "StarBuffer.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ImageTest, BgrConvertSwapsChannels) {
  Image bgr(1, 1, PixelFormat::BGRA32);
  bgr.setrgb(0, 0, Vec4B(10, 20, 30, 255));
  EXPECT_EQ(bgr.getrgb(0, 0), Vec4B(10, 20, 30, 255));
  EXPECT_EQ(bgr.convert(PixelFormat::RGBA32).get(0, 0), Vec4B(10, 20, 30, 255));
}

TEST(ImageTest, BgrPngRoundTrip) {
  Image bgr(1, 1, PixelFormat::BGR24);
  bgr.setrgb(0, 0, Vec3B(10, 20, 30));
  auto buffer = make_shared<Buffer>();
  bgr.writePng(buffer);
  buffer->seek(0);
  Image read = Image::readPng(buffer);
  EXPECT_EQ(read.getrgb(0, 0).vec3(), Vec3B(10, 20, 30));
}

TEST(ImageTest, FloatPngRoundTrip) {
  Image f(1, 1, PixelFormat::RGBA_F);
  f.setF(0, 0, Color::rgbaf(2.0f, 0.5f, 0.0f, 1.0f));
  auto buffer = make_shared<Buffer>();
  f.writePng(buffer);
  buffer->seek(0);
  Image read = Image::readPng(buffer);
  EXPECT_EQ(read.get(0, 0)[0], 255);
  EXPECT_NEAR(read.get(0, 0)[1], 127, 1);
}
