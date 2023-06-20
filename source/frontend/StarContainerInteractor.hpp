#ifndef STAR_CONTAINER_INTERACTION_HPP
#define STAR_CONTAINER_INTERACTION_HPP

#include "StarContainerEntity.hpp"

namespace Star {

STAR_CLASS(ContainerInteractor);

typedef List<ItemPtr> ContainerResult;

class ContainerInteractor {
public:
  void openContainer(ContainerEntityPtr containerEntity);
  void closeContainer();

  bool containerOpen() const;

  // Returns NullEntityId if no container is open
  EntityId openContainerId() const;

  // Throws exception if there is no currently open container.
  ContainerEntityPtr const& openContainer() const;

  List<ContainerResult> pullContainerResults();

  void swapInContainer(size_t slot, ItemPtr const& items);
  void addToContainer(ItemPtr const& items);
  void takeFromContainerSlot(size_t slot, size_t count);
  void applyAugmentInContainer(size_t slot, ItemPtr const& augment);

  void startCraftingInContainer();
  void stopCraftingInContainer();
  void burnContainer();
  void clearContainer();

private:
  static ContainerResult resultFromItem(ItemPtr const& items);

  mutable ContainerEntityPtr m_openContainer;
  List<RpcPromise<ContainerResult>> m_pendingResults;
};

}

#endif
