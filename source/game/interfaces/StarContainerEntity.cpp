#include "StarContainerEntity.hpp"
#include "StarItemBag.hpp"

namespace Star {

size_t ContainerEntity::containerSize() const {
  return itemBag()->size();
}

List<ItemPtr> ContainerEntity::containerItems() const {
  return itemBag()->items();
}

}

