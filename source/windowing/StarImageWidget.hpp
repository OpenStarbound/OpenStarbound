#ifndef STAR_IMAGE_WIDGET_HPP
#define STAR_IMAGE_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ImageWidget);
class ImageWidget : public Widget {
public:
  ImageWidget(String const& image = {});

  bool interactive() const override;
  void setImage(String const& image);
  void setScale(float scale);
  void setRotation(float rotation);
  String image() const;

  void setDrawables(List<Drawable> drawables);
  Vec2I offset();
  void setOffset(Vec2I const& offset);
  bool centered();
  void setCentered(bool centered);
  bool trim();
  void setTrim(bool trim);

  void setMaxSize(Vec2I const& size);
  void setMinSize(Vec2I const& size);

  RectI screenBoundRect() const override;

protected:
  void renderImpl() override;

private:
  void transformDrawables();

  List<Drawable> m_baseDrawables;
  List<Drawable> m_drawables;
  bool m_centered;
  bool m_trim;
  float m_scale;
  float m_rotation;
  Vec2I m_offset;
  Vec2I m_maxSize;
  Vec2I m_minSize;
};

}

#endif
