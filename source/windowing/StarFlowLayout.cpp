#include "StarFlowLayout.hpp"

namespace Star {

FlowLayout::FlowLayout() : m_wrap(true) {}

void FlowLayout::update() {
  Layout::update();

  int consumedWidth = 0;
  int rowHeight = 0;
  Vec2I currentOffset = {0, size()[1]};
  for (auto child : m_members) {
    if (m_wrap && consumedWidth + child->size()[0] > size()[0] && consumedWidth != 0) { // wrapping
      currentOffset[0] = 0;
      consumedWidth = 0;
      currentOffset[1] -= rowHeight + m_spacing[1];
    }
    if (rowHeight < child->size()[1]) {
      rowHeight = child->size()[1];
    }
    child->setPosition(Vec2I{currentOffset[0], currentOffset[1] - child->size()[1]});
    consumedWidth += child->size()[0] + m_spacing[0];
    currentOffset[0] = consumedWidth;
  }
}

void FlowLayout::setSpacing(Vec2I const& spacing) {
  m_spacing = spacing;
}

void FlowLayout::setWrapping(bool wrap) {
  m_wrap = wrap;
}

}
