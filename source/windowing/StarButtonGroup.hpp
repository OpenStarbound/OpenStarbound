#pragma once

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ButtonWidget);
STAR_CLASS(ButtonGroup);
STAR_CLASS(ButtonGroupWidget);

// Manages group of buttons in which *at most* a single button can be checked
// at any time.
class ButtonGroup {
public:
  friend class ButtonWidget;

  static int const NoButton = -1;

  // Callback is called when any child buttons checked state is changed, and
  // its parameter is the button being checked.
  void setCallback(WidgetCallbackFunc callback);

  ButtonWidget* button(int id) const;
  List<ButtonWidget*> buttons() const;
  size_t buttonCount() const;

  int addButton(ButtonWidget* button, int id = NoButton);
  void removeButton(ButtonWidget* button);

  int id(ButtonWidget* button) const;

  void select(int id);

  // Will return null if no button is checked.
  ButtonWidget* checkedButton() const;
  // Will return NoButton if no button is checked.
  int checkedId() const;

  // when true it is not required for one of the buttons to be selected
  bool toggle() const;
  void setToggle(bool toggleMode);

protected:
  // Should be called by child button widgets when they are changed from
  // unchecked to checked.
  void wasChecked(ButtonWidget* self);

private:
  WidgetCallbackFunc m_callback;
  Map<int, ButtonWidget*> m_buttons;
  Map<ButtonWidget*, int> m_buttonIds;
  bool m_toggle;
};

class ButtonGroupWidget : public ButtonGroup, public Widget {};
}
