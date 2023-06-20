#ifndef STAR_QUEST_TRACKER_HPP
#define STAR_QUEST_TRACKER_HPP

#include "StarPane.hpp"

namespace Star {

STAR_CLASS(LabelWidget);
STAR_CLASS(ImageWidget);
STAR_CLASS(ImageStretchWidget);
STAR_CLASS(ProgressWidget);
STAR_CLASS(Quest);

class QuestTrackerPane : public Pane {
public:
  QuestTrackerPane();

  bool sendEvent(InputEvent const& event) override;
  void update() override;

  void setQuest(QuestPtr const& quest);

private:
  void setExpanded(bool expanded);

  ImageWidgetPtr m_frame;
  ImageStretchWidgetPtr m_expandedFrame;

  LabelWidgetPtr m_questObjectiveList;

  ImageWidgetPtr m_compassFrame;
  ImageWidgetPtr m_compass;

  ImageWidgetPtr m_progressFrame;
  ProgressWidgetPtr m_progress;

  int m_expandedFrameMinHeight;
  int m_expandedFramePadding;

  float m_compassDirection;
  float m_compassSpeed;
  float m_compassAcceleration;
  float m_compassFriction;

  QuestPtr m_currentQuest;
  bool m_expanded;

  String m_compassFrameImage;
  String m_expandedCompassFrameImage;

  String m_progressFrameImage;
  String m_expandedProgressFrameImage;

  String m_incompleteObjectiveTemplate;
  String m_completeObjectiveTemplate;
};

}

#endif
