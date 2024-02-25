#pragma once

#include "StarWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarPortraitWidget.hpp"
#include "StarLabelWidget.hpp"

namespace Star {

STAR_CLASS(Player);

STAR_CLASS(LargeCharPlateWidget);
class LargeCharPlateWidget : public ButtonWidget {
public:
  LargeCharPlateWidget(WidgetCallbackFunc mainCallback, PlayerPtr player = PlayerPtr());

  void mouseOut() override;

  void setPlayer(PlayerPtr player = PlayerPtr());

  void enableDelete(WidgetCallbackFunc const& callback);
  void disableDelete();

  virtual bool sendEvent(InputEvent const& event) override;

protected:
  virtual void renderImpl() override;

private:
  PlayerPtr m_player;

  PortraitWidgetPtr m_portrait;
  Vec2I m_portraitOffset;
  float m_portraitScale;

  String m_playerPlateHover;
  String m_noPlayerPlate;
  String m_noPlayerPlateHover;
  String m_playerPlate;

  LabelWidgetPtr m_playerName;
  LabelWidgetPtr m_playerPhrase;
  LabelWidgetPtr m_modeName;
  LabelWidgetPtr m_mode;

  ButtonWidgetPtr m_delete;

  Vec2I m_playerNameOffset;
  Vec2I m_playerPhraseOffset;
  Vec2I m_modeNameOffset;
  Vec2I m_modeOffset;
  Vec2I m_deleteOffset;

  String m_createCharText;
  Color m_createCharTextColor;

  Color m_regularTextColor;
  Color m_disabledTextColor;
};

}
