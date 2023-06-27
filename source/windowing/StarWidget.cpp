#include "StarWidget.hpp"
#include "StarPane.hpp"

#include "StarLabelWidget.hpp"
#include "StarFlowLayout.hpp"

namespace Star {

Widget::Widget() {
  m_parent = nullptr;
  m_context = GuiContext::singletonPtr();
  m_visible = true;
  m_focus = false;
  m_doScissor = true;
  m_container = false;
  m_mouseTransparent = false;
}

Widget::~Widget() {
  removeAllChildren();
}

void Widget::update() {
  for (auto widget : m_members)
    widget->update();
}

GuiContext* Widget::context() const {
  return m_context;
}

void Widget::render(RectI const& region) {
  if (!m_visible)
    return;

  if (!setupDrawRegion(region))
    return;

  renderImpl();
  drawChildren();
}

void Widget::renderImpl() {}

void Widget::drawChildren() {
  for (auto child : m_members)
    child->render(m_drawingArea);
}

Vec2I Widget::position() const {
  return m_position + m_drawingOffset;
}

Vec2I Widget::relativePosition() const {
  return m_position;
}

bool Widget::setupDrawRegion(RectI const& region) {
  RectI scissorRect;
  if (m_doScissor) {
    scissorRect = getScissorRect();
  } else {
    scissorRect = noScissor();
  }
  m_drawingArea = scissorRect.limited(region);
  if (m_drawingArea.isEmpty())
    return false;

  m_context->setInterfaceScissorRect(m_drawingArea);
  return true;
}

Vec2I Widget::drawingOffset() const {
  return m_drawingOffset;
}

void Widget::setDrawingOffset(Vec2I const& offset) {
  m_drawingOffset = offset;
}

RectI Widget::getScissorRect() const {
  return screenBoundRect();
}

RectI Widget::noScissor() const {
  return RectI::inf();
}

void Widget::disableScissoring() {
  m_doScissor = false;
}

void Widget::enableScissoring() {
  m_doScissor = true;
}

void Widget::setPosition(Vec2I const& position) {
  m_position = position;
}

Vec2I Widget::size() const {
  return m_size;
}

void Widget::setSize(Vec2I const& size) {
  m_size = size;
}

RectI Widget::relativeBoundRect() const {
  return RectI::withSize(relativePosition(), size());
}

RectI Widget::screenBoundRect() const {
  return relativeBoundRect().translated(screenPosition() - relativePosition());
}

void Widget::determineSizeFromChildren() {
  Vec2I max;

  for (auto& child : m_members) {
    Vec2I childMax = child->position() + child->size();
    max = max.piecewiseMax(childMax);
  }

  setSize(max);
}

void Widget::markAsContainer() {
  m_container = true;
}

KeyboardCaptureMode Widget::keyboardCaptured() const {
  if (active()) {
    for (auto const& member : m_members) {
      auto mode = member->keyboardCaptured();
      if (mode != KeyboardCaptureMode::None)
        return mode;
    }
  }
  return KeyboardCaptureMode::None;
}

void Widget::setData(Json const& data) {
  m_data = data;
}

Json const& Widget::data() {
  return m_data;
}

Vec2I Widget::screenPosition() const {
  if (m_parent) {
    return m_parent->screenPosition() + position();
  } else {
    return position();
  }
}

bool Widget::inMember(Vec2I const& position) const {
  if (!m_visible)
    return false;

  if (m_mouseTransparent)
    return false;

  if (!m_drawingArea.isNull() && !m_drawingArea.contains(Vec2I::floor(position)))
    return false;

  if (m_container) {
    for (auto child : m_members)
      if (child->inMember(position))
        return true;
  } else {
    return screenBoundRect().contains(position);
  }

  return false;
}

bool Widget::sendEvent(InputEvent const& event) {
  if (!m_visible)
    return false;

  for (auto child : reverseIterate(m_members)) {
    if (child->sendEvent(event))
      return true;
  }

  return false;
}

void Widget::mouseOver() {}

void Widget::mouseOut() {}

void Widget::mouseReturnStillDown() {}

void Widget::setMouseTransparent(bool transparent) {
  m_mouseTransparent = transparent;
}

bool Widget::mouseTransparent() {
  return m_mouseTransparent;
}

Widget* Widget::parent() const {
  return m_parent;
}

void Widget::setParent(Widget* parent) {
  m_parent = parent;
}

void Widget::show() {
  m_visible = true;
}

void Widget::hide() {
  m_visible = false;
}

void Widget::toggleVisibility() {
  m_visible = !m_visible;
}

void Widget::setVisibility(bool visibility) {
  if (visibility)
    show();
  else
    hide();
}

bool Widget::active() const {
  return m_visible;
}

bool Widget::interactive() const {
  return true;
}

bool Widget::hasFocus() const {
  return m_focus;
}

void Widget::focus() {
  m_focus = true;
  if (auto w = window())
    w->setFocus(this);
}

void Widget::blur() {
  m_focus = false;
  if (window())
    window()->removeFocus(this);
}

unsigned Widget::windowHeight() const {
  return context()->windowHeight();
}

unsigned Widget::windowWidth() const {
  return context()->windowWidth();
}

Vec2I Widget::windowSize() const {
  return Vec2I(context()->windowSize());
}

Pane* Widget::window() {
  if (m_parent)
    return m_parent->window();
  return nullptr;
}

Pane const* Widget::window() const {
  if (m_parent)
    return m_parent->window();
  return nullptr;
}

void Widget::addChild(String const& name, WidgetPtr member) {
  member->setName(name);
  m_members.push_back(member);
  m_memberHash[member->name()] = member;
  member->setParent(this);
}

void Widget::addChildAt(String const& name, WidgetPtr member, size_t at) {
  if (at > m_members.size())
    throw GuiException("Attempted to insert item after the end of the list.");

  m_members.insert(m_members.begin() + at, member);
  m_memberHash[name] = member;
  member->setName(name);
  member->setParent(this);
}

bool Widget::removeChild(Widget* member) {
  m_memberHash.erase(member->name());
  for (auto child = m_members.begin(); child != m_members.end(); ++child) {
    if (child->get() == member) {
      (*child)->setParent(nullptr);
      m_members.erase(child);
      return true;
    } else {
      if ((*child)->removeChild(member))
        return true;
    }
  }

  return false;
}

bool Widget::removeChild(String const& name) {
  m_memberHash.erase(name);
  for (auto child = m_members.begin(); child != m_members.end(); ++child) {
    auto ptr = *child;
    if (ptr->name() == name) {
      m_members.erase(child);
      ptr->setParent(nullptr);
      return true;
    }
  }

  return false;
}

bool Widget::removeChildAt(size_t at) {
  if (at >= m_members.size())
    return false;

  m_memberHash.erase(m_members.at(at)->name());

  m_members.at(at)->setParent(nullptr);
  m_members.erase(m_members.begin() + at);
  return true;
}

void Widget::removeAllChildren() {
  for (auto child : m_members)
    child->setParent(nullptr);

  m_members.clear();
  m_memberHash.clear();
}

bool Widget::containsChild(String const& name) {
  if (fetchChild(name))
    return true;
  return false;
}

WidgetPtr Widget::fetchChild(String const& name) {
  WidgetPtr res;
  if (name.contains(".")) {
    StringList nameList = name.split(".", 1);
    if (auto child = m_memberHash.value(nameList[0], {}))
      return child->fetchChild(nameList[1]);
  } else {
    if (auto child = m_memberHash.value(name, {}))
      return child;
  }
  return {};
}

WidgetPtr Widget::findChild(String const& name) {
  if (auto found = fetchChild(name))
    return found;
  for (auto const& child : m_members) {
    if (auto found = child->findChild(name))
      return found;
  }
  return {};
}

WidgetPtr Widget::childPtr(Widget const* child) const {
  for (auto const& m : m_members) {
    if (m.get() == child)
      return m;
    if (auto c = m->childPtr(child))
      return c;
  }
  return {};
}

WidgetPtr Widget::getChildAt(Vec2I const& pos) {
  for (auto child : reverseIterate(m_members)) {
    if (child->inMember(pos)) {
      auto res = child->getChildAt(pos);
      if (res) {
        return res;
      }
      return child;
    }
  }

  return {};
}

size_t Widget::numChildren() const {
  return m_members.size();
}

WidgetPtr Widget::getChildNum(size_t num) const {
  return m_members.at(num);
}

String const& Widget::name() const {
  return m_name;
}

void Widget::setName(String const& name) {
  m_name = name;
}

String Widget::fullName() const {
  if (m_parent) {
    return m_parent->fullName() + "." + name();
  }

  return name();
}

String Widget::toStringImpl(int indentLevel) const {
  auto leader = String(" ") * indentLevel;
  String childrenString;
  for (auto child : m_members) {
    childrenString.append(child->toStringImpl(indentLevel + 4));
  }
  String output = strf(R"OUTPUT(%s%s : {
%s  address : %p,
%s  visible : %s,
%s  position : %s,
%s  size : %s,
%s  children : {
%s
%s  }
%s}
)OUTPUT",
      leader,
      m_name,
      leader,
      (void*)this,
      leader,
      m_visible ? "true" : "false",
      leader,
      m_position,
      leader,
      m_size,
      leader,
      childrenString,
      leader,
      leader);

  return output;
}

bool Widget::setLabel(String const& name, String const& value) {
  if (containsChild(name)) {
    auto child = fetchChild(name);
    if (auto label = as<LabelWidget>(child)) {
      label->setText(value);
      return true;
    }
  }
  return false;
}

std::ostream& operator<<(std::ostream& os, Widget const& widget) {
  os << widget.toStringImpl(0);
  return os;
}

}
