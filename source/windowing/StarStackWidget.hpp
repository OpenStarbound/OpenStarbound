#ifndef STAR_STACK_WIDGET_HPP
#define STAR_STACK_WIDGET_HPP
#include "StarWidget.hpp"
#include "StarEither.hpp"

namespace Star {

STAR_CLASS(StackWidget);
class StackWidget : public Widget {
public:
  void showPage(size_t page);
  void showPage(String const& name);

  Either<size_t, String> currentPage() const;

  virtual void addChild(String const& name, WidgetPtr member) override;

private:
  WidgetPtr m_shownPage;
  Either<size_t, String> m_page;
};

}

#endif
