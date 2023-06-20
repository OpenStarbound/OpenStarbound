#ifndef STAR_CONTAINER_OBJECT_HPP
#define STAR_CONTAINER_OBJECT_HPP

#include "StarItemBag.hpp"
#include "StarObject.hpp"
#include "StarWeightedPool.hpp"
#include "StarContainerEntity.hpp"
#include "StarItemRecipe.hpp"

namespace Star {

STAR_CLASS(ContainerObject);

class ContainerObject : public Object, public virtual ContainerEntity {
public:
  ContainerObject(ObjectConfigConstPtr config, Json const& parameters);

  void init(World* world, EntityId entityId, EntityMode mode) override;

  void update(uint64_t currentStep) override;
  void render(RenderCallback* renderCallback) override;

  void destroy(RenderCallback* renderCallback) override;
  InteractAction interact(InteractRequest const& request) override;

  Maybe<Json> receiveMessage(ConnectionId sendingConnection, String const& message, JsonArray const& args) override;

  Json containerGuiConfig() const override;
  String containerDescription() const override;
  String containerSubTitle() const override;
  ItemDescriptor iconItem() const override;

  ItemBagConstPtr itemBag() const override;

  void containerOpen() override;
  void containerClose() override;

  void startCrafting() override;
  void stopCrafting() override;
  bool isCrafting() const override;
  float craftingProgress() const override;

  void burnContainerContents() override;

  RpcPromise<ItemPtr> addItems(ItemPtr const& items) override;
  RpcPromise<ItemPtr> putItems(size_t slot, ItemPtr const& items) override;
  RpcPromise<ItemPtr> takeItems(size_t slot, size_t count = NPos) override;
  RpcPromise<ItemPtr> swapItems(size_t slot, ItemPtr const& items, bool tryCombine = true) override;
  RpcPromise<ItemPtr> applyAugment(size_t slot, ItemPtr const& augment) override;
  RpcPromise<bool> consumeItems(ItemDescriptor const& descriptor) override;
  RpcPromise<bool> consumeItems(size_t slot, size_t count) override;
  RpcPromise<List<ItemPtr>> clearContainer() override;

protected:
  void getNetStates(bool initial) override;
  void setNetStates() override;

  void readStoredData(Json const& diskStore) override;
  Json writeStoredData() const override;

private:
  typedef std::function<void(ContainerObject*)> ContainerCallback;

  ItemRecipe recipeForMaterials(List<ItemPtr> const& inputItems);
  void tickCrafting();

  ItemPtr doAddItems(ItemPtr const& items);
  ItemPtr doStackItems(ItemPtr const& items);
  ItemPtr doPutItems(size_t slot, ItemPtr const& items);
  ItemPtr doTakeItems(size_t slot, size_t count = NPos);
  ItemPtr doSwapItems(size_t slot, ItemPtr const& items, bool tryCombine = true);
  ItemPtr doApplyAugment(size_t slot, ItemPtr const& augment);
  bool doConsumeItems(ItemDescriptor const& descriptor);
  bool doConsumeItems(size_t slot, size_t count);
  List<ItemPtr> doClearContainer();

  template<typename T>
  RpcPromise<T> addSlavePromise(String const& message, JsonArray const& args, function<T(Json)> converter);

  void itemsUpdated();

  NetElementInt m_opened;

  NetElementBool m_crafting;
  NetElementFloat m_craftingProgress;

  ItemBagPtr m_items;
  NetElementBytes m_itemsNetState;

  // master only

  bool m_initialized;
  int m_count;
  int m_currentState;
  int64_t m_animationFrameCooldown;
  int64_t m_autoCloseCooldown;

  ItemRecipe m_goalRecipe;

  bool m_itemsUpdated;
  bool m_runUpdatedCallback;

  ContainerCallback m_containerCallback;

  EpochTimer m_ageItemsTimer;

  List<ItemPtr> m_lostItems;
};

}

#endif
