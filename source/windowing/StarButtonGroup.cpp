#include "StarButtonGroup.hpp"
#include "StarButtonWidget.hpp"

namespace Star {

void ButtonGroup::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

ButtonWidget* ButtonGroup::button(int id) const {
  return m_buttons.value(id);
}

List<ButtonWidget*> ButtonGroup::buttons() const {
  return m_buttons.values();
}

size_t ButtonGroup::buttonCount() const {
  return m_buttons.size();
}

int ButtonGroup::addButton(ButtonWidget* button, int id) {
  if (!button)
    return NoButton;
  else if (m_buttonIds.contains(button))
    return m_buttonIds.get(button);

  // If we are auto-generating an id, start at the last id and work forward.
  if (id == NoButton && !m_buttons.empty())
    id = (prev(m_buttons.end()))->first;

  while (m_buttons.contains(id))
    ++id;

  m_buttons[id] = button;
  m_buttonIds[button] = id;
  return id;
}

void ButtonGroup::removeButton(ButtonWidget* button) {
  if (m_buttonIds.contains(button)) {
    m_buttons.remove(m_buttonIds.get(button));
    m_buttonIds.remove(button);
  }
}

int ButtonGroup::id(ButtonWidget* button) const {
  if (m_buttonIds.contains(button))
    return m_buttonIds.get(button);
  else
    return NoButton;
}

ButtonWidget* ButtonGroup::checkedButton() const {
  for (auto const& pair : m_buttons) {
    if (pair.second->isChecked())
      return pair.second;
  }
  return {};
}

int ButtonGroup::checkedId() const {
  return id(checkedButton());
}

void ButtonGroup::select(int id) {
  auto b = button(id);
  if (!b->isChecked())
    b->check();
}

void ButtonGroup::wasChecked(ButtonWidget* self) {
  for (auto const& pair : m_buttons) {
    if (pair.second != self)
      pair.second->setChecked(false);
  }

  if (m_callback)
    m_callback(self);
}

bool ButtonGroup::toggle() const {
  return m_toggle;
}

void ButtonGroup::setToggle(bool toggleMode) {
  m_toggle = toggleMode;
}

}
