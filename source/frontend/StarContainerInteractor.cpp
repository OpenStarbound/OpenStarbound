#include "StarContainerInteractor.hpp"

namespace Star {

void ContainerInteractor::openContainer(ContainerEntityPtr containerEntity) {
  if (m_openContainer && m_openContainer->inWorld())
    m_openContainer->containerClose();
  m_openContainer = move(containerEntity);
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

ContainerEntityPtr const& ContainerInteractor::openContainer() const {
  if (m_openContainer && !m_openContainer->inWorld())
    m_openContainer = {};

  if (!m_openContainer)
    throw StarException("ContainerInteractor has no open container");

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
  m_pendingResults.append(openContainer()->swapItems(slot, items).wrap(resultFromItem));
}

void ContainerInteractor::addToContainer(ItemPtr const& items) {
  m_pendingResults.append(openContainer()->addItems(items).wrap(resultFromItem));
}

void ContainerInteractor::takeFromContainerSlot(size_t slot, size_t count) {
  m_pendingResults.append(openContainer()->takeItems(slot, count).wrap(resultFromItem));
}

void ContainerInteractor::applyAugmentInContainer(size_t slot, ItemPtr const& augment) {
  m_pendingResults.append(openContainer()->applyAugment(slot, augment).wrap(resultFromItem));
}

void ContainerInteractor::startCraftingInContainer() {
  openContainer()->startCrafting();
}

void ContainerInteractor::stopCraftingInContainer() {
  openContainer()->stopCrafting();
}

void ContainerInteractor::burnContainer() {
  openContainer()->burnContainerContents();
}

void ContainerInteractor::clearContainer() {
  m_pendingResults.append(openContainer()->clearContainer());
}

ContainerResult ContainerInteractor::resultFromItem(ItemPtr const& item) {
  if (item)
    return {item};
  else
    return {};
}

}
