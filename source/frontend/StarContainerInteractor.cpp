#include "StarContainerInteractor.hpp"
namespace Star {

void ContainerInteractor::openContainer(ContainerEntityPtr containerEntity) {
  if (m_openContainer && m_openContainer->inWorld())
    m_openContainer->containerClose();
  m_openContainer = std::move(containerEntity);
  if (m_openContainer) {
    starAssert(m_openContainer->inWorld());
    m_openContainer->containerOpen();
  }
}

void ContainerInteractor::closeContainer() {
  openContainer({});
}

bool ContainerInteractor::containerOpen() const {
  return openContainerId() != NullEntityId;
}

EntityId ContainerInteractor::openContainerId() const {
  if (m_openContainer && !m_openContainer->inWorld())
    m_openContainer = {};

  if (m_openContainer)
    return m_openContainer->entityId();

  return NullEntityId;
}

// Make sure to check if this is valid when you use it!
ContainerEntityPtr const& ContainerInteractor::openContainer() const {
  if (m_openContainer && !m_openContainer->inWorld())
    m_openContainer = {};

  return m_openContainer;
}

List<ContainerResult> ContainerInteractor::pullContainerResults() {
  List<List<ItemPtr>> results;
  eraseWhere(m_pendingResults, [&results](auto& promise) {
      if (auto res = promise.result())
        results.append(res.take());
      return promise.finished();
    });
  return results;
}

void ContainerInteractor::swapInContainer(size_t slot, ItemPtr const& items) {
  auto container = openContainer();
  if (!container)
    return;

  m_pendingResults.append(container->swapItems(slot, items).wrap(resultFromItem));
}

void ContainerInteractor::addToContainer(ItemPtr const& items) {
  auto container = openContainer();
  if (!container)
    return;
    
  m_pendingResults.append(container->addItems(items).wrap(resultFromItem));
}

void ContainerInteractor::takeFromContainerSlot(size_t slot, size_t count) {
  auto container = openContainer();
  if (!container)
    return;
    
  m_pendingResults.append(container->takeItems(slot, count).wrap(resultFromItem));
}

void ContainerInteractor::applyAugmentInContainer(size_t slot, ItemPtr const& augment) {
  auto container = openContainer();
  if (!container)
    return;
    
  m_pendingResults.append(container->applyAugment(slot, augment).wrap(resultFromItem));
}

void ContainerInteractor::startCraftingInContainer() {
  auto container = openContainer();
  if (!container)
    return;
    
  container->startCrafting();
}

void ContainerInteractor::stopCraftingInContainer() {
  auto container = openContainer();
  if (!container)
    return;
    
  container->stopCrafting();
}

void ContainerInteractor::burnContainer() {
  auto container = openContainer();
  if (!container)
    return;
    
  container->burnContainerContents();
}

void ContainerInteractor::clearContainer() {
  auto container = openContainer();
  if (!container)
    return;
    
  m_pendingResults.append(container->clearContainer());
}

ContainerResult ContainerInteractor::resultFromItem(ItemPtr const& item) {
  if (item)
    return {item};
  else
    return {};
}

}
