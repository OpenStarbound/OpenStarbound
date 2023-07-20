#include "StarQuestTracker.hpp"
#include "StarMathCommon.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarProgressWidget.hpp"
#include "StarVerticalLayout.hpp"
#include "StarQuests.hpp"
#include "StarLogging.hpp"

namespace Star {

QuestTrackerPane::QuestTrackerPane() {
  auto assets = Root::singleton().assets();
  auto config = assets->json("/interface/questtracker/questtracker.config");

  m_progressFrameImage = config.getString("progressFrameImage");
  m_expandedProgressFrameImage = config.getString("expandedProgressFrameImage");

  m_compassFrameImage = config.getString("compassFrameImage");
  m_expandedCompassFrameImage = config.getString("expandedCompassFrameImage");

  m_incompleteObjectiveTemplate = config.getString("incompleteObjectiveTemplate");
  m_completeObjectiveTemplate = config.getString("completeObjectiveTemplate");

  m_expandedFrameMinHeight = config.getInt("expandedFrameMinHeight");
  m_expandedFramePadding = config.getInt("expandedFramePadding");

  m_compassDirection = 0;
  m_compassSpeed = 0;
  m_compassAcceleration = config.getFloat("compassAcceleration");
  m_compassFriction = config.getFloat("compassFriction");

  GuiReader reader;
  reader.construct(config.get("paneLayout"), this);

  m_frame = fetchChild<ImageWidget>("imgFrame");
  m_expandedFrame = fetchChild<ImageStretchWidget>("imgFrameExpanded");

  m_compassFrame = fetchChild<ImageWidget>("imgCompassFrame");
  m_compass = fetchChild<ImageWidget>("imgCompass");

  m_questObjectiveList = fetchChild<LabelWidget>("lblQuestObjectiveList");

  m_progress = fetchChild<ProgressWidget>("questProgress");
  m_progressFrame = fetchChild<ImageWidget>("imgProgressFrame");

  setExpanded(false);

  // allow things like the compass and progress bar to be drawn outside the background
  disableScissoring();
  m_progressFrame->disableScissoring();
}

bool QuestTrackerPane::sendEvent(InputEvent const& event) {
  if (Pane::sendEvent(event))
    return true;

  if (event.is<MouseButtonDownEvent>() && event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Left) {
    auto mousePos = *context()->mousePosition(event);
    if ((m_expanded && m_expandedFrame->inMember(mousePos)) ||
        (!m_expanded && m_frame->inMember(mousePos))) {
      setExpanded(!m_expanded);
      return true;
    }
  }

  return false;
}

void QuestTrackerPane::update(float dt) {
  if (m_currentQuest) {
    if (auto objectiveList = m_currentQuest->objectiveList()) {
      if (objectiveList->size() == 0) {
        m_questObjectiveList->hide();
      } else {
        m_questObjectiveList->show();
        if (m_expanded) {
          String listText = "";
          for (auto objective : objectiveList.value()) {
            if (objective.get(1).toBool())
              listText += m_completeObjectiveTemplate.replaceTags(StringMap<String>{{"objective", objective.get(0).toString()}});
            else
              listText += m_incompleteObjectiveTemplate.replaceTags(StringMap<String>{{"objective", objective.get(0).toString()}});
          }
          m_questObjectiveList->setText(listText);
        } else {
          String displayObjective = "";
          for (auto objective : objectiveList.value()) {
            if (displayObjective.empty() || !objective.get(1).toBool()) {
              if (objective.get(1).toBool()) {
                displayObjective = m_completeObjectiveTemplate.replaceTags(StringMap<String>{{"objective", objective.get(0).toString()}});
              } else {
                displayObjective = m_incompleteObjectiveTemplate.replaceTags(StringMap<String>{{"objective", objective.get(0).toString()}});
                break;
              }
            }
          }
          m_questObjectiveList->setText(displayObjective);
        }
      }
    } else {
      m_questObjectiveList->hide();
      m_questObjectiveList->setText("");
    }

    if (m_expanded) {
      m_expandedFrame->show();
      m_frame->hide();

      int frameHeight = max(m_expandedFrameMinHeight, m_questObjectiveList->size()[1] + m_expandedFramePadding * 2);
      m_expandedFrame->setSize(Vec2I(m_expandedFrame->size()[0], frameHeight));
      m_expandedFrame->setPosition(Vec2I(m_expandedFrame->relativePosition()[0], -frameHeight));
    } else {
      m_frame->show();
      m_expandedFrame->hide();
    }

    if (auto progress = m_currentQuest->progress()) {
      m_progress->show();
      m_progressFrame->show();

      if (m_expanded) {
        m_progressFrame->setImage(m_expandedProgressFrameImage);

        m_progress->setDrawingOffset(Vec2I(0, -m_expandedFrame->size()[1]));
        m_progressFrame->setDrawingOffset(Vec2I(0, -m_expandedFrame->size()[1]));
      } else {
        m_progressFrame->setImage(m_progressFrameImage);

        m_progress->setDrawingOffset(Vec2I(0, -m_frame->size()[1]));
        m_progressFrame->setDrawingOffset(Vec2I(0, -m_frame->size()[1]));
      }

      m_progress->setCurrentProgressLevel(*progress);
    } else {
      m_progress->hide();
      m_progressFrame->hide();
    }

    if (auto compassDirection = m_currentQuest->compassDirection()) {
      m_compass->show();
      m_compassFrame->show();

      if (m_expanded || m_currentQuest->progress())
        m_compassFrame->setImage(m_expandedCompassFrameImage);
      else
        m_compassFrame->setImage(m_compassFrameImage);

      float compassDiff = pfmod(*compassDirection - m_compassDirection, (float)Constants::pi * 2);
      if (abs(compassDiff) < m_compassAcceleration && abs(m_compassSpeed) < m_compassAcceleration) {
        m_compassSpeed = 0;
        m_compassDirection = *compassDirection;
      } else {
        if (abs(compassDiff) > Constants::pi)
          compassDiff -= copysign(2 * Constants::pi, compassDiff);
        float diffRatio = abs(compassDiff) / Constants::pi;
        float speedChange = m_compassAcceleration * diffRatio;

        m_compassSpeed += copysign(speedChange, compassDiff);
        m_compassSpeed -= m_compassSpeed * m_compassFriction;
        m_compassDirection += m_compassSpeed;
      }

      m_compass->setRotation(m_compassDirection);
    } else {
      m_compass->hide();
      m_compassFrame->hide();
    }
  }

  Pane::update(dt);
}

void QuestTrackerPane::setQuest(QuestPtr const& quest) {
  m_currentQuest = quest;
}

void QuestTrackerPane::setExpanded(bool expanded) {
  m_expanded = expanded;
}

}
