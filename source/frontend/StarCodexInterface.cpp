#include "StarCodexInterface.hpp"
#include "StarCodex.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarPlayer.hpp"
#include "StarLabelWidget.hpp"
#include "StarListWidget.hpp"
#include "StarStackWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarButtonWidget.hpp"
#include "StarButtonGroup.hpp"
#include "StarAssets.hpp"

namespace Star {

CodexInterface::CodexInterface(PlayerPtr player) {
  m_player = player;

  auto assets = Root::singleton().assets();

  GuiReader reader;

  reader.registerCallback("close", [=](Widget*) { dismiss(); });
  reader.registerCallback("prevButton", [=](Widget*) { backwardPage(); });
  reader.registerCallback("nextButton", [=](Widget*) { forwardPage(); });
  reader.registerCallback("selectCodex", [=](Widget*) { showSelectedContents(); });
  reader.registerCallback("updateSpecies", [=](Widget*) { updateSpecies(); });

  reader.construct(assets->json("/interface/windowconfig/codex.config:paneLayout"), this);

  m_speciesTabs = fetchChild<ButtonGroupWidget>("speciesTabs");
  m_selectLabel = fetchChild<LabelWidget>("selectLabel");
  m_titleLabel = fetchChild<LabelWidget>("titleLabel");
  m_bookList = fetchChild<ListWidget>("scrollArea.bookList");
  m_pageContent = fetchChild<LabelWidget>("pageText");
  m_pageLabelWidget = fetchChild<LabelWidget>("pageLabel");
  m_pageNumberWidget = fetchChild<LabelWidget>("pageNum");
  m_prevPageButton = fetchChild<ButtonWidget>("prevButton");
  m_nextPageButton = fetchChild<ButtonWidget>("nextButton");

  m_selectText = assets->json("/interface/windowconfig/codex.config:selectText").toString();

  m_currentPage = 0;
  updateSpecies();
  setupPageText();
}

void CodexInterface::show() {
  Pane::show();
  updateCodexList();
}

void CodexInterface::tick(float) {
  updateCodexList();
}

void CodexInterface::showSelectedContents() {
  if (m_bookList->selectedItem() == NPos || m_bookList->selectedItem() >= m_codexList.size())
    return;

  showContents(m_codexList[m_bookList->selectedItem()].first);
}

void CodexInterface::showContents(String const& codexId) {
  CodexConstPtr result;
  for (auto entry : m_codexList)
    if (entry.first->id() == codexId) {
      result = entry.first;
      break;
    }
  if (result)
    showContents(result);
}

void CodexInterface::showContents(CodexConstPtr codex) {
  if (m_player->codexes()->markCodexRead(codex->id()))
    updateCodexList();
  m_currentCodex = codex;
  m_currentPage = 0;
  setupPageText();
}

void CodexInterface::forwardPage() {
  if (m_currentCodex && m_currentPage < m_currentCodex->pageCount() - 1) {
    ++m_currentPage;
    setupPageText();
  }
}

void CodexInterface::backwardPage() {
  if (m_currentCodex && m_currentPage > 0) {
    --m_currentPage;
    setupPageText();
  }
}

bool CodexInterface::showNewCodex() {
  if (auto newCodex = m_player->codexes()->firstNewCodex()) {
    for (auto button : m_speciesTabs->buttons()) {
      if (button->data().getString("species") == newCodex->species()) {
        m_speciesTabs->select(m_speciesTabs->id(button));
        break;
      }
    }
    showContents(newCodex);
    return true;
  }

  return false;
}

void CodexInterface::updateSpecies() {
  String newSpecies = "other";
  if (auto speciesButton = m_speciesTabs->checkedButton())
    newSpecies = speciesButton->data().getString("species");
  if (newSpecies != m_currentSpecies) {
    m_currentCodex = {};
    m_currentSpecies = newSpecies;
    m_bookList->clearSelected();
    setupPageText();
  }
  m_selectLabel->setText(m_selectText.replaceTags(StringMap<String>{{"species", m_currentSpecies}}).titleCase());
}

void CodexInterface::setupPageText() {
  if (m_currentCodex) {
    m_pageContent->setText(m_currentCodex->page(m_currentPage));
    m_pageLabelWidget->show();
    m_pageNumberWidget->setText(strf("{} of {}", m_currentPage + 1, m_currentCodex->pageCount()));
    m_titleLabel->setText(m_currentCodex->title());
    m_nextPageButton->setEnabled(m_currentPage < m_currentCodex->pageCount() - 1);
    m_prevPageButton->setEnabled(m_currentPage > 0);
  } else {
    m_pageContent->setText("");
    m_pageLabelWidget->hide();
    m_pageNumberWidget->setText("");
    m_titleLabel->setText("");
    m_nextPageButton->disable();
    m_prevPageButton->disable();
  }
}

void CodexInterface::updateCodexList() {
  auto newCodexList = m_player->codexes()->codexes();
  filter(newCodexList, [&](auto const& p) {
      return p.first->species() == m_currentSpecies;
    });
  if (m_codexList != newCodexList) {
    m_bookList->removeAllChildren();
    m_codexList = newCodexList;
    for (auto entry : m_codexList) {
      auto newEntry = m_bookList->addItem();
      newEntry->fetchChild<LabelWidget>("bookName")->setText(entry.first->title());
      newEntry->fetchChild<ImageWidget>("bookIcon")->setImage(entry.first->icon());
    }
  }
}

}
