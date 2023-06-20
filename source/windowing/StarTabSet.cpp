#include "StarTabSet.hpp"
#include "StarButtonWidget.hpp"
#include "StarStackWidget.hpp"
#include "StarFlowLayout.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarLexicalCast.hpp"
#include "StarImageMetadataDatabase.hpp"

namespace Star {

TabSetWidget::TabSetWidget(TabSetConfig const& tabSetConfig) {
  m_tabSetConfig = tabSetConfig;

  m_tabBar = make_shared<FlowLayout>();
  m_tabBar->setSpacing(m_tabSetConfig.tabButtonSpacing);
  Widget::addChild("tabBar", m_tabBar);

  m_stack = make_shared<StackWidget>();
  addChild("tabs", m_stack);

  markAsContainer();
}

void TabSetWidget::setSize(Vec2I const& size) {
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  auto tabHeight = max({imgMetadata->imageSize(m_tabSetConfig.tabButtonBaseImage).y(),
      imgMetadata->imageSize(m_tabSetConfig.tabButtonHoverImage).y(),
      imgMetadata->imageSize(m_tabSetConfig.tabButtonPressedImage).y(),
      imgMetadata->imageSize(m_tabSetConfig.tabButtonBaseImageSelected).y(),
      imgMetadata->imageSize(m_tabSetConfig.tabButtonHoverImageSelected).y(),
      imgMetadata->imageSize(m_tabSetConfig.tabButtonPressedImageSelected).y()});

  Widget::setSize(Vec2I(size.x(), max<int>(size.y(), tabHeight)));

  m_tabBar->setSize({size.x(), tabHeight});
  m_tabBar->setPosition({0, size.y() - tabHeight});
  m_stack->setSize({size.x(), size.y() - tabHeight});
}

void TabSetWidget::addTab(String const& widgetName, WidgetPtr widget, String const& title) {
  auto newButton = make_shared<ButtonWidget>();
  newButton->setImages(
      m_tabSetConfig.tabButtonBaseImage, m_tabSetConfig.tabButtonHoverImage, m_tabSetConfig.tabButtonPressedImage);
  newButton->setCheckedImages(m_tabSetConfig.tabButtonBaseImageSelected,
      m_tabSetConfig.tabButtonHoverImageSelected,
      m_tabSetConfig.tabButtonPressedImageSelected);
  newButton->setCheckable(true);
  newButton->setText(title);
  newButton->setTextOffset(m_tabSetConfig.tabButtonTextOffset);
  newButton->setPressedOffset(m_tabSetConfig.tabButtonPressedOffset);

  size_t pageForButton = m_tabBar->numChildren();
  newButton->setCallback([this, pageForButton](Widget*) { tabSelect(pageForButton); });

  m_tabBar->addChild(toString(pageForButton), newButton);
  m_stack->addChild(widgetName, widget);

  if (!m_lastSelected)
    tabSelect(0);
}

size_t TabSetWidget::tabCount() const {
  return m_tabBar->numChildren();
}

void TabSetWidget::tabSelect(size_t page) {
  if (m_lastSelected != page) {
    m_lastSelected = page;
    m_stack->showPage(page);
    for (size_t i = 0; i < m_tabBar->numChildren(); ++i) {
      if (i == page)
        m_tabBar->getChildNum<ButtonWidget>(i)->setChecked(true);
      else
        m_tabBar->getChildNum<ButtonWidget>(i)->setChecked(false);
    }
    if (m_callback)
      m_callback(this);
  } else {
    m_tabBar->getChildNum<ButtonWidget>(page)->setChecked(true);
  }
}

size_t TabSetWidget::selectedTab() const {
  return m_lastSelected.value(NPos);
}

void TabSetWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = move(callback);
}

}
