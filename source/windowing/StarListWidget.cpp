#include "StarListWidget.hpp"
#include "StarGuiReader.hpp"
#include "StarJsonExtra.hpp"
#include "StarRandom.hpp"
#include "StarImageWidget.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

ListWidget::ListWidget(Json const& schema) : m_schema(schema) {
  m_selectedItem = NPos;
  m_columns = 1;
  setSchema(m_schema);
  updateSizeAndPosition();
}

ListWidget::ListWidget() {
  m_selectedItem = NPos;
  m_columns = 1;
  updateSizeAndPosition();
}

RectI ListWidget::relativeBoundRect() const {
  if (m_fillDown)
    return RectI::withSize(relativePosition() - Vec2I(0, size()[1]), size());
  else
    return RectI::withSize(relativePosition(), size());
}

void ListWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = std::move(callback);
}

bool ListWidget::sendEvent(InputEvent const& event) {
  if (!m_visible)
    return false;

  for (size_t i = m_members.size(); i != 0; --i) {
    auto child = m_members[i - 1];
    if (child->sendEvent(event)
        || (event.is<MouseButtonDownEvent>() && child->inMember(*context()->mousePosition(event))
              && event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Left)) {
      setSelected(i - 1);
      return true;
    }
    setHovered(i - 1, event.is<MouseMoveEvent>() && child->inMember(*context()->mousePosition(event)));
  }

  return false;
}

void ListWidget::setSchema(Json const& schema) {
  clear();
  m_schema = schema;
  try {
    m_selectedBG = schema.getString("selectedBG", "");
    m_unselectedBG = schema.getString("unselectedBG", "");
    m_hoverBG = schema.getString("hoverBG", "");
    m_disabledBG = schema.getString("disabledBG", "");
    if (m_disabledBG.empty() && !m_unselectedBG.empty())
      m_disabledBG = m_unselectedBG + Root::singleton().assets()->json("/interface.config:disabledButton").toString();
    m_spacing = jsonToVec2I(schema.get("spacing"));
    m_memberSize = jsonToVec2I(schema.get("memberSize"));
  } catch (JsonException const& e) {
    throw GuiException(strf("Missing required value in map: {}", outputException(e, false)));
  }
  updateSizeAndPosition();
}

WidgetPtr ListWidget::addItem() {
  auto newItem = constructWidget();
  addChild(toString(Random::randu64()), newItem);
  updateSizeAndPosition();

  return newItem;
}

WidgetPtr ListWidget::addItem(size_t at) {
  auto newItem = constructWidget();
  addChildAt(toString(Random::randu64()), newItem, at);
  updateSizeAndPosition();

  if (m_selectedItem != NPos && at <= m_selectedItem)
    setSelected(m_selectedItem + 1);

  return newItem;
}

WidgetPtr ListWidget::addItem(WidgetPtr existingItem) {
  addChild(toString(Random::randu64()), existingItem);
  updateSizeAndPosition();

  return existingItem;
}

WidgetPtr ListWidget::constructWidget() {
  WidgetPtr newItem = make_shared<Widget>();
  m_reader.construct(m_schema.get("listTemplate"), newItem.get());
  newItem->setSize(m_memberSize);
  m_doScissor ? newItem->enableScissoring() : newItem->disableScissoring();
  return newItem;
}

void ListWidget::updateSizeAndPosition() {
  int rows = m_members.size() % m_columns ? m_members.size() / m_columns + 1 : m_members.size() / m_columns;
  for (size_t i = 0; i < m_members.size(); i++) {
    Vec2I currentOffset;
    int col = i % m_columns;
    int row = rows - (i / m_columns) - 1;
    if (m_fillDown)
      row -= rows;
    currentOffset = Vec2I((m_memberSize[0] + m_spacing[0]) * col, (m_memberSize[1] + m_spacing[1]) * row);
    if (!m_fillDown)
      currentOffset[1] += m_spacing[1];
    m_members[i]->setPosition(currentOffset);
  }
  if (m_members.size()) {
    auto width = (m_memberSize[0] + m_spacing[0]) * m_columns;
    auto height = (m_memberSize[1] + m_spacing[1]) * rows;
    setSize(Vec2I(width, height));
  } else {
    setSize(Vec2I());
  }
}

void ListWidget::setEnabled(size_t pos, bool enabled) {
  if (pos != NPos && pos < listSize()) {
    if (enabled) {
      m_disabledItems.remove(pos);
      if (auto bgWidget = itemAt(pos)->fetchChild<ImageWidget>("background"))
        bgWidget->setImage(pos == m_selectedItem ? m_selectedBG : m_unselectedBG);
    } else {
      m_disabledItems.add(pos);
      if (m_selectedItem == pos)
        clearSelected();
      if (auto bgWidget = itemAt(pos)->fetchChild<ImageWidget>("background"))
        bgWidget->setImage(m_disabledBG);
    }
  }
}

void ListWidget::setHovered(size_t pos, bool hovered) {
  if (m_hoverBG == "")
    return;

  if (pos != m_selectedItem && pos < listSize() && !m_disabledItems.contains(pos)) {
    if (auto bgWidget = itemAt(pos)->fetchChild<ImageWidget>("background")) {
      if (hovered)
        bgWidget->setImage(m_hoverBG);
      else
        bgWidget->setImage(m_unselectedBG);
    }
  }
}

void ListWidget::setSelected(size_t pos) {
  if ((m_selectedItem != NPos) && (m_selectedItem < listSize())) {
    if (auto bgWidget = selectedWidget()->fetchChild<ImageWidget>("background"))
      bgWidget->setImage(m_unselectedBG);
  }

  if (!m_disabledItems.contains(pos) && m_selectedItem != pos) {
    m_selectedItem = pos;
    if (m_callback)
      m_callback(this);
  }

  if (m_selectedItem != NPos) {
    if (auto bgWidget = selectedWidget()->fetchChild<ImageWidget>("background"))
      bgWidget->setImage(m_selectedBG);
  }
}

void ListWidget::clearSelected() {
  setSelected(NPos);
}

void ListWidget::setSelectedWidget(WidgetPtr selected) {
  auto offset = itemPosition(selected);

  if (offset == NPos) {
    throw GuiException("Attempted to select item not in list.");
  }

  setSelected(offset);
}

void ListWidget::registerMemberCallback(String const& name, WidgetCallbackFunc const& callback) {
  m_reader.registerCallback(name, callback);
}

void ListWidget::setFillDown(bool fillDown) {
  m_fillDown = fillDown;
}

void ListWidget::setColumns(uint64_t columns) {
  m_columns = columns;
}

void ListWidget::removeItem(size_t at) {
  removeChildAt(at);
  if (m_selectedItem == at)
    setSelected(NPos);
  else if (m_selectedItem != NPos && m_selectedItem > at)
    setSelected(m_selectedItem - 1);

  updateSizeAndPosition();
}

void ListWidget::removeItem(WidgetPtr item) {
  auto offset = itemPosition(item);

  if (offset == NPos) {
    throw GuiException("Attempted to remove item not in list.");
  }

  removeItem(offset);
}

void ListWidget::clear() {
  setSelected(NPos);
  removeAllChildren();
  updateSizeAndPosition();
}

size_t ListWidget::selectedItem() const {
  return m_selectedItem;
}

size_t ListWidget::itemPosition(WidgetPtr item) const {
  size_t offset = NPos;
  for (size_t i = 0; i < m_members.size(); ++i) {
    if (m_members[i] == item) {
      offset = i;
      break;
    }
  }
  return offset;
}

WidgetPtr ListWidget::itemAt(size_t n) const {
  if (n < m_members.size()) {
    return m_members[n];
  } else {
    return {};
  }
}

WidgetPtr ListWidget::selectedWidget() const {
  return itemAt(m_selectedItem);
}

List<WidgetPtr> const& ListWidget::list() const {
  return m_members;
}

size_t ListWidget::listSize() const {
  return numChildren();
}

}
