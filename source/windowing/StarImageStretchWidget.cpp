#include "StarImageStretchWidget.hpp"

namespace Star {

ImageStretchWidget::ImageStretchWidget(ImageStretchSet const& imageStretchSet, GuiDirection direction)
  : m_imageStretchSet(imageStretchSet), m_direction(direction) {

}

void ImageStretchWidget::setImageStretchSet(String const& beginImage, String const& innerImage, String const& endImage) {
  m_imageStretchSet.begin = beginImage;
  m_imageStretchSet.inner = innerImage;
  m_imageStretchSet.end = endImage;
}

void ImageStretchWidget::renderImpl() {
  context()->drawImageStretchSet(m_imageStretchSet, RectF(screenBoundRect()), m_direction);
}

}
