#include "StarItemDrop.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarEntityRendering.hpp"
#include "StarWorld.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarPlayer.hpp"
#include "StarMaterialItem.hpp"

namespace Star {

ItemDropPtr ItemDrop::createRandomizedDrop(ItemPtr const& item, Vec2F const& position, bool eternal) {
  if (!item)
    return {};

  auto idconfig = Root::singleton().assets()->json("/itemdrop.config");

  ItemDropPtr itemDrop = make_shared<ItemDrop>(item);
  auto offset = Vec2F(idconfig.getFloat("randomizedDistance"), 0).rotate(Constants::pi * 2.0 * Random::randf());
  offset[1] = fabs(offset[1]);
  itemDrop->setPosition(position + offset / TilePixels);
  itemDrop->setVelocity(offset * idconfig.getFloat("randomizedSpeed"));
  itemDrop->setEternal(eternal);

  return itemDrop;
}

ItemDropPtr ItemDrop::createRandomizedDrop(ItemDescriptor const& descriptor, Vec2F const& position, bool eternal) {
  if (!descriptor || descriptor.isEmpty())
    return {};

  auto itemDatabase = Root::singleton().itemDatabase();
  auto itemDrop = createRandomizedDrop(itemDatabase->item(descriptor), position);
  itemDrop->setEternal(eternal);

  return itemDrop;
}

ItemDropPtr ItemDrop::throwDrop(ItemPtr const& item, Vec2F const& position, Vec2F const& velocity, Vec2F const& direction, bool eternal) {
  if (!item)
    return {};

  auto idconfig = Root::singleton().assets()->json("/itemdrop.config");

  ItemDropPtr itemDrop = make_shared<ItemDrop>(item);
  itemDrop->setPosition(position);
  if (direction != Vec2F())
    itemDrop->setVelocity(velocity + vnorm(direction) * idconfig.getFloat("throwSpeed"));

  itemDrop->setEternal(eternal);
  itemDrop->setIntangibleTime(idconfig.getFloat("throwIntangibleTime"));

  return itemDrop;
}

ItemDropPtr ItemDrop::throwDrop(ItemDescriptor const& itemDescriptor, Vec2F const& position, Vec2F const& velocity, Vec2F const& direction, bool eternal) {
  if (!itemDescriptor || itemDescriptor.isEmpty())
    return {};

  auto itemDatabase = Root::singleton().itemDatabase();
  auto itemDrop = throwDrop(itemDatabase->item(itemDescriptor), position, velocity, direction);
  itemDrop->setEternal(eternal);

  return itemDrop;
}

ItemDrop::ItemDrop(ItemPtr item)
  : ItemDrop() {
  m_item = std::move(item);

  updateCollisionPoly();

  m_owningEntity.set(NullEntityId);
  m_mode.set(Mode::Available);
  m_itemDescriptor.set(m_item->descriptor());
}

ItemDrop::ItemDrop(Json const& diskStore)
  : ItemDrop() {
  Root::singleton().itemDatabase()->diskLoad(diskStore.get("item"), m_item);
  m_movementController.setPosition(jsonToVec2F(diskStore.get("position")));
  m_mode.set(ModeNames.getLeft(diskStore.getString("mode")));
  m_eternal = diskStore.getBool("eternal");
  m_dropAge = EpochTimer(diskStore.get("dropAge"));
  m_ageItemsTimer = EpochTimer(diskStore.get("ageItemsTimer"));

  updateCollisionPoly();
  m_owningEntity.set(NullEntityId);
  m_itemDescriptor.set(m_item->descriptor());
}

ItemDrop::ItemDrop(ByteArray store, NetCompatibilityRules rules) : ItemDrop() {
  DataStreamBuffer ds(std::move(store));
  ds.setStreamCompatibilityVersion(rules);

  Root::singleton().itemDatabase()->loadItem(ds.read<ItemDescriptor>(), m_item);
  ds.read(m_eternal);
  ds.read(m_dropAge);
  ds.read(m_intangibleTimer);

  updateCollisionPoly();
}

Json ItemDrop::diskStore() const {
  auto itemDatabase = Root::singleton().itemDatabase();
  return JsonObject{
    {"item", itemDatabase->diskStore(m_item)},
    {"position", jsonFromVec2F(m_movementController.position())},
    {"mode", ModeNames.getRight(m_mode.get())},
    {"eternal", m_eternal},
    {"dropAge", m_dropAge.toJson()},
    {"ageItemsTimer", m_ageItemsTimer.toJson()}
  };
}

ByteArray ItemDrop::netStore(NetCompatibilityRules rules) const {
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(rules);

  ds.write(itemSafeDescriptor(m_item));
  ds.write(m_eternal);
  ds.write(m_dropAge);
  ds.write(m_intangibleTimer);

  return ds.takeData();
}

EntityType ItemDrop::entityType() const {
  return EntityType::ItemDrop;
}

void ItemDrop::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);

  m_movementController.init(world);
}

void ItemDrop::uninit() {
  Entity::uninit();
  m_movementController.uninit();
}

String ItemDrop::name() const {
  if (m_item)
    return m_item->name();
  return Entity::name();
}

String ItemDrop::description() const {
  return m_item->description();
}

pair<ByteArray, uint64_t> ItemDrop::writeNetState(uint64_t fromVersion, NetCompatibilityRules rules) {
  return m_netGroup.writeNetState(fromVersion, rules);
}

void ItemDrop::readNetState(ByteArray data, float interpolationTime, NetCompatibilityRules rules) {
  m_netGroup.readNetState(data, interpolationTime, rules);
}

void ItemDrop::enableInterpolation(float extrapolationHint) {
  m_netGroup.enableNetInterpolation(extrapolationHint);
  m_mode.disableNetInterpolation();
  m_owningEntity.disableNetInterpolation();
}

void ItemDrop::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

Vec2F ItemDrop::position() const {
  return m_movementController.position();
}

RectF ItemDrop::metaBoundBox() const {
  return m_boundBox;
}

bool ItemDrop::ephemeral() const {
  return true;
}

RectF ItemDrop::collisionArea() const {
  return m_boundBox;
}

void ItemDrop::update(float dt, uint64_t) {
  m_dropAge.update(world()->epochTime());

  if (isMaster()) {
    if (m_owningEntity.get() != NullEntityId) {
      updateTaken(true);
    } else {
      // Rarely, check for other drops near us and combine with them if possible.
      if (canTake() && m_mode.get() == Mode::Available && Random::randf() < m_combineChance) {
        world()->findEntity(RectF::withCenter(position(), Vec2F::filled(m_combineRadius)), [&](EntityPtr const& entity) {
            if (auto closeDrop = as<ItemDrop>(entity)) {
              // Make sure not to try to merge with ourselves here.
              if (closeDrop.get() != this && closeDrop->canTake()
                  && vmag(position() - closeDrop->position()) < m_combineRadius) {
                if (m_item->couldStack(closeDrop->item()) == closeDrop->item()->count()) {
                  m_item->stackWith(closeDrop->take());
                  m_dropAge.setElapsedTime(min(m_dropAge.elapsedTime(), closeDrop->m_dropAge.elapsedTime()));

                  // Average the position and velocity of the drop we merged
                  // with
                  m_movementController.setPosition(m_movementController.position()
                      + world()->geometry().diff(closeDrop->position(), m_movementController.position()) / 2.0f);
                  m_movementController.setVelocity((m_movementController.velocity() + closeDrop->velocity()) / 2.0f);
                  return true;
                }
              }
            }
            return false;
          });
      }

      MovementParameters parameters;
      parameters.collisionEnabled = true;
      parameters.gravityEnabled = true;
      m_movementController.applyParameters(parameters);
    }

    m_movementController.tickMaster(dt);

    m_intangibleTimer.tick(dt);
    m_ageItemsTimer.update(world()->epochTime());

    if ((m_mode.get() == Mode::Intangible || m_mode.get() == Mode::Available) && m_movementController.atWorldLimit())
      m_mode.set(Mode::Dead);
    if (m_mode.get() == Mode::Intangible && m_intangibleTimer.ready())
      m_mode.set(Mode::Available);
    if (!m_eternal && m_mode.get() == Mode::Available && m_dropAge.elapsedTime() > m_item->timeToLive())
      m_mode.set(Mode::Dead);
    if (m_mode.get() == Mode::Taken && m_dropAge.elapsedTime() > m_afterTakenLife)
      m_mode.set(Mode::Dead);

    if (m_mode.get() <= Mode::Available && m_ageItemsTimer.elapsedTime() > m_ageItemsEvery) {
      if (Root::singleton().itemDatabase()->ageItem(m_item, m_ageItemsTimer.elapsedTime())) {
        m_itemDescriptor.set(m_item->descriptor());
        updateCollisionPoly();
      }
      m_ageItemsTimer.setElapsedTime(0.0);
    }
  } else {
    if (m_itemDescriptor.pullUpdated())
      Root::singleton().itemDatabase()->loadItem(m_itemDescriptor.get(), m_item);
    m_netGroup.tickNetInterpolation(dt);
    if (m_owningEntity.get() != NullEntityId) {
      if (!isMaster() && m_dropAge.elapsedTime() > 1.0f)
        m_owningEntity.set(NullEntityId);
      else {
        updateTaken(false);
        m_movementController.tickMaster(dt);
      }
    }
    else {
      m_movementController.tickSlave(dt);
    }
  }

  if (world()->isClient()) {
    SpatialLogger::logPoly("world",
      m_movementController.collisionBody(),
      (canTake() ? Color::Green : Color::Red).toRgba());
  }
}

bool ItemDrop::shouldDestroy() const {
  return m_mode.get() == Mode::Dead || (m_item->empty() && m_owningEntity.get() == NullEntityId);
}

void ItemDrop::render(RenderCallback* renderCallback) {
  if (m_mode.get() != Mode::Taken) {
    Color beamColor;
    switch (m_item->rarity()) {
    case Rarity::Uncommon:
      beamColor = Color::rgb(87, 255, 81);
      break;
    case Rarity::Rare:
      beamColor = Color::rgb(87, 220, 255);
      break;
    case Rarity::Legendary:
      beamColor = Color::rgb(176, 81, 255);
      break;
    case Rarity::Essential:
      beamColor = Color::rgb(255, 255, 81);
      break;
    default:
      beamColor = Color::White;
    }

    beamColor.setAlphaF(0.8f);
    Line2F line = { Vec2F(), Vec2F(0.0f, 1.0f + m_boundBox.height() / 2) };
    float width = min(2.0f, m_boundBox.width() * TilePixels);
    auto drawable = Drawable::makeLine(line, width, beamColor, position());
    (drawable.linePart().endColor = beamColor)->setAlphaF(0.0f);
    drawable.fullbright = true;
    renderCallback->addDrawable(std::move(drawable), RenderLayerItemDrop);
  }

  if (!m_drawables) {
    if (auto mat = as<MaterialItem>(m_item.get())) {
      m_drawables = mat->generatedPreview(Vec2I(position().floor()));
      m_overForeground = true;
    }
    else
      m_drawables = m_item->dropDrawables();

    if (Directives dropDirectives = m_config.getString("directives", "")) {
      for (auto& drawable : *m_drawables) {
        if (drawable.isImage())
          drawable.imagePart().addDirectives(dropDirectives, true);
      }
    }
  }
  EntityRenderLayer renderLayer = m_mode.get() == Mode::Taken || m_overForeground ? RenderLayerForegroundTile : RenderLayerItemDrop;
  Vec2F dropPosition = position();
  for (Drawable drawable : *m_drawables) {
    drawable.position += dropPosition;
    renderCallback->addDrawable(std::move(drawable), renderLayer);
  }
}

void ItemDrop::renderLightSources(RenderCallback* renderCallback) {
  LightSource light;
  light.type = LightType::Spread;
  light.color = Vec3F::filled(20.f / 255.f);
  light.position = position();
  renderCallback->addLightSource(std::move(light));
}


ItemPtr ItemDrop::item() const {
  return m_item;
}

void ItemDrop::setEternal(bool eternal) {
  m_eternal = eternal;
}

void ItemDrop::setIntangibleTime(float intangibleTime) {
  m_intangibleTimer = GameTimer(intangibleTime);
  if (m_mode.get() == Mode::Available)
    m_mode.set(Mode::Intangible);
}

bool ItemDrop::canTake() const {
  return m_mode.get() == Mode::Available && m_owningEntity.get() == NullEntityId && !m_item->empty();
}

ItemPtr ItemDrop::takeBy(EntityId entityId, float timeOffset) {
  if (canTake()) {
    m_owningEntity.set(entityId);
    m_dropAge.setElapsedTime(timeOffset);
    m_mode.set(Mode::Taken);
    setPersistent(false);

    return m_item->take();
  } else {
    return {};
  }
}

ItemPtr ItemDrop::take() {
  if (canTake()) {
    m_mode.set(Mode::Taken);
    return m_item->take();
  } else {
    return {};
  }
}

void ItemDrop::setPosition(Vec2F const& position) {
  m_movementController.setPosition(position);
}

Vec2F ItemDrop::velocity() const {
  return m_movementController.velocity();
}

void ItemDrop::setVelocity(Vec2F const& velocity) {
  m_movementController.setVelocity(velocity);
}

EnumMap<ItemDrop::Mode> const ItemDrop::ModeNames{
    {ItemDrop::Mode::Intangible, "Intangible"},
    {ItemDrop::Mode::Available, "Available"},
    {ItemDrop::Mode::Taken, "Taken"},
    {ItemDrop::Mode::Dead, "Dead"}};

ItemDrop::ItemDrop() {
  setPersistent(true);

  m_config = Root::singleton().assets()->json("/itemdrop.config");

  MovementParameters parameters = MovementParameters(m_config.get("movementSettings", JsonObject()));
  if (!parameters.physicsEffectCategories)
    parameters.physicsEffectCategories = StringSet({"itemdrop"});
  if (parameters.collisionPoly)
    m_defaultBoundBox = parameters.collisionPoly->boundBox();
  else
    m_defaultBoundBox = RectF{-0.499, -0.499, 0.499, 0.499};
  m_boundBox = m_defaultBoundBox;

  m_movementController.applyParameters(parameters);

  m_netGroup.addNetElement(&m_mode);
  m_netGroup.addNetElement(&m_owningEntity);
  m_netGroup.addNetElement(&m_movementController);
  m_netGroup.addNetElement(&m_itemDescriptor);

  m_afterTakenLife = m_config.getFloat("afterTakenLife");
  m_overheadTime = m_config.getFloat("overheadTime");
  m_pickupDistance = m_config.getFloat("pickupDistance");
  m_velocity = m_config.getFloat("velocity");
  m_velocityApproach = m_config.getFloat("velocityApproach");
  m_overheadApproach = m_config.getFloat("overheadApproach");
  m_overheadOffset = Vec2F(m_config.getFloat("overheadRandomizedDistance"), 0).rotate(Constants::pi * 2.0 * Random::randf());

  m_combineChance = m_config.getFloat("combineChance");
  m_combineRadius = m_config.getFloat("combineRadius");
  m_ageItemsEvery = m_config.getDouble("ageItemsEvery", 10);

  m_eternal = false;
  m_overForeground = false;
}

void ItemDrop::updateCollisionPoly() {
  /* // currently disabled due to causing items to get stuck
  if (!as<MaterialItem>(m_item.get())) {
    m_boundBox = Drawable::boundBoxAll(m_item->dropDrawables(), true);
    m_boundBox.rangeSetIfEmpty(m_defaultBoundBox);
    MovementParameters parameters;
    parameters.collisionPoly = PolyF(collisionArea());
    m_movementController.applyParameters(parameters);
  }
  */
}


void ItemDrop::updateTaken(bool master) {
  if (auto owningEntity = world()->entity(m_owningEntity.get())) {
    Vec2F position = m_movementController.position();
    bool overhead = m_dropAge.elapsedTime() < m_overheadTime;
    Vec2F targetPosition = owningEntity->position();
    if (overhead) {
      targetPosition += m_overheadOffset;
      auto rect = owningEntity->collisionArea();
      if (!rect.isNull())
        targetPosition[1] += rect.yMax() + 1.5f;
      else
        targetPosition[1] += 1.5f;
    }
    Vec2F diff = world()->geometry().diff(targetPosition, position);
    float magnitude = diff.magnitude();
    Vec2F velocity = diff.normalized() * m_velocity * min(1.0f, magnitude);
    if (auto playerEntity = as<Player>(owningEntity))
      velocity += playerEntity->velocity();
    m_movementController.approachVelocity(velocity, overhead ? m_overheadApproach : m_velocityApproach);
    if (master && !overhead && magnitude < m_pickupDistance)
      m_mode.set(Mode::Dead);

  } else if (master) {
    // Our owning entity left, disappear quickly
    m_mode.set(Mode::Dead);
  }

  MovementParameters parameters;
  parameters.maxMovementPerStep = 1000.0f;
  parameters.collisionEnabled = false;
  parameters.gravityEnabled = false;
  m_movementController.applyParameters(parameters);
}

}
