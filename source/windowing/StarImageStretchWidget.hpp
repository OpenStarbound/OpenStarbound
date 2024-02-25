#pragma once

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
