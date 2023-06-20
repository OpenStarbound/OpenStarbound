#ifndef STAR_SLIDER_BAR_HPP
#define STAR_SLIDER_BAR_HPP
#include "StarWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarImageWidget.hpp"

namespace Star {

// This class only does left right right now.  If you need advanced
// multiorientation physics please
// implement it in.
class SliderBarWidget : public Widget {
public:
  SliderBarWidget(String const& grid, bool showSpinner = true);

  void setJogImages(String const& baseImage, String const& hoverImage = "", String const& pressedImage = "", String const& disabledImage = "");

  void setRange(int low, int high, int delta);
  void setRange(Vec2I const& range, int delta);
  void setVal(int val, bool callbackIfChanged = true);
  int val() const;

  void setEnabled(bool enabled);

  void setCallback(WidgetCallbackFunc callback);

  virtual void update() override;

  virtual bool sendEvent(InputEvent const& event) override;

private:
  void leftCallback();
  void rightCallback();

  ButtonWidgetPtr m_leftButton;
  ButtonWidgetPtr m_rightButton;
  ImageWidgetPtr m_grid;
  ButtonWidgetPtr m_jog;
  int m_low;
  int m_high;
  int m_delta;
  int m_val;

  bool m_updateJog;

  Vec2I m_savedJogPos;
  Vec2I m_jogDragPos;
  bool m_jogDragActive;

  bool m_enabled;

  WidgetCallbackFunc m_callback;
};
typedef shared_ptr<SliderBarWidget> SliderBarWidgetPtr;
}

#endif
