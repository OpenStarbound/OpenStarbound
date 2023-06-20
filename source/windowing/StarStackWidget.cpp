#include "StarStackWidget.hpp"

namespace Star {

void StackWidget::showPage(size_t page) {
  if (m_shownPage)
    m_shownPage->hide();
  m_shownPage = m_members[page];
  m_page = makeLeft(page);
  if (m_shownPage)
    m_shownPage->show();
}

void StackWidget::showPage(String const& name) {
  if (m_shownPage)
    m_shownPage->hide();
  m_shownPage = m_memberHash.get(name);
  m_page = makeRight(name);
  if (m_shownPage)
    m_shownPage->show();
}

Either<size_t, String> StackWidget::currentPage() const {
  return m_page;
}

void StackWidget::addChild(String const& name, WidgetPtr member) {
  Widget::addChild(name, member);
  if (m_members.size() != 1)
    member->hide();
  else
    showPage(0);
}

}
