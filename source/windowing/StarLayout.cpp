#include "StarLayout.hpp"

namespace Star {

Layout::Layout() {
  markAsContainer();
}

void Layout::update(float dt) {
  Widget::update(dt);
}

}
