#ifndef STAR_CODEX_INTERFACE_HPP
#define STAR_CODEX_INTERFACE_HPP

#include "StarPane.hpp"
#include "StarPlayerCodexes.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(JsonRpcInterface);
STAR_CLASS(StackWidget);
STAR_CLASS(ListWidget);
STAR_CLASS(LabelWidget);
STAR_CLASS(ButtonWidget);
STAR_CLASS(ButtonGroupWidget);
STAR_CLASS(Codex);

STAR_CLASS(CodexInterface);
class CodexInterface : public Pane {
public:
  CodexInterface(PlayerPtr player);

  virtual void show() override;
  virtual void tick(float dt) override;

  void showTitles();
  void showSelectedContents();
  void showContents(String const& codexId);
  void showContents(CodexConstPtr codex);

  void forwardPage();
  void backwardPage();

  bool showNewCodex();

private:
  void updateSpecies();
  void setupPageText();
  void updateCodexList();

  StackWidgetPtr m_stack;

  ListWidgetPtr m_bookList;

  CodexConstPtr m_currentCodex;
  size_t m_currentPage;

  ButtonGroupWidgetPtr m_speciesTabs;
  LabelWidgetPtr m_selectLabel;
  LabelWidgetPtr m_titleLabel;
  LabelWidgetPtr m_pageContent;
  LabelWidgetPtr m_pageLabelWidget;
  LabelWidgetPtr m_pageNumberWidget;
  ButtonWidgetPtr m_prevPageButton;
  ButtonWidgetPtr m_nextPageButton;
  ButtonWidgetPtr m_backButton;

  String m_selectText;
  String m_currentSpecies;

  PlayerPtr m_player;
  List<PlayerCodexes::CodexEntry> m_codexList;
};

}

#endif
