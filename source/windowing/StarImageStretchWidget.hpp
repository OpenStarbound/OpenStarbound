#ifndef STAR_IMAGE_STRETCH_WIDGET_HPP
#define STAR_IMAGE_STRETCH_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ImageStretchWidget);

class ImageStretchWidget : public Widget {
public:
  ImageStretchWidget(ImageStretchSet const& imageStretchSet, GuiDirection direction);
  virtual ~ImageStretchWidget() {}

protected:
  virtual void renderImpl();

private:
  ImageStretchSet m_imageStretchSet;
  GuiDirection m_direction;
};

}

#endif
