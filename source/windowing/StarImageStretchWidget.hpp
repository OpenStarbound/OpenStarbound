#pragma once

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ImageStretchWidget);

class ImageStretchWidget : public Widget {
public:
  ImageStretchWidget(ImageStretchSet const& imageStretchSet, GuiDirection direction);
  void setImageStretchSet(String const& beginImage, String const& innerImage, String const& endImage);

  virtual ~ImageStretchWidget() {}

protected:
  virtual void renderImpl();

private:
  ImageStretchSet m_imageStretchSet;
  GuiDirection m_direction;
};

}
