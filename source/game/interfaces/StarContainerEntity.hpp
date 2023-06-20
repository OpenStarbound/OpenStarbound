#ifndef STAR_CONTAINER_ENTITY_HPP
#define STAR_CONTAINER_ENTITY_HPP

#include "StarGameTypes.hpp"
#include "StarTileEntity.hpp"
#include "StarItemDescriptor.hpp"
#include "StarRpcPromise.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ItemBag);
STAR_CLASS(ContainerEntity);

// All container methods may be called on both master and slave entities.
class ContainerEntity : public virtual TileEntity {
public:
  size_t containerSize() const;
  List<ItemPtr> containerItems() const;

  virtual Json containerGuiConfig() const = 0;
  virtual String containerDescription() const = 0;
  virtual String containerSubTitle() const = 0;
  virtual ItemDescriptor iconItem() const = 0;

  virtual ItemBagConstPtr itemBag() const = 0;

  virtual void containerOpen() = 0;
  virtual void containerClose() = 0;

  virtual void startCrafting() = 0;
  virtual void stopCrafting() = 0;
  virtual bool isCrafting() const = 0;
  virtual float craftingProgress() const = 0;

  virtual void burnContainerContents() = 0;

  virtual RpcPromise<ItemPtr> addItems(ItemPtr const& items) = 0;
  virtual RpcPromise<ItemPtr> putItems(size_t slot, ItemPtr const& items) = 0;
  virtual RpcPromise<ItemPtr> takeItems(size_t slot, size_t count = NPos) = 0;
  virtual RpcPromise<ItemPtr> swapItems(size_t slot, ItemPtr const& items, bool tryCombine = true) = 0;
  virtual RpcPromise<ItemPtr> applyAugment(size_t slot, ItemPtr const& augment) = 0;
  virtual RpcPromise<bool> consumeItems(ItemDescriptor const& descriptor) = 0;
  virtual RpcPromise<bool> consumeItems(size_t slot, size_t count) = 0;
  virtual RpcPromise<List<ItemPtr>> clearContainer() = 0;
};

}

#endif
