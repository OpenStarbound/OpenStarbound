#pragma once

#include "StarColor.hpp"
#include "StarCollisionBlock.hpp"
#include "StarMaterialRenderProfile.hpp"
#include "StarTileDamage.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_CLASS(ParticleConfig);
STAR_CLASS(MaterialDatabase);

STAR_EXCEPTION(MaterialException, StarException);

struct LiquidMaterialInteraction {
  float consumeLiquid;
  MaterialId transformTo;
  bool topOnly;
};

struct LiquidModInteraction {
  float consumeLiquid;
  ModId transformTo;
  bool topOnly;
};

class MaterialDatabase {
public:
  MaterialDatabase();

  StringList materialNames() const;
  bool isMetaMaterialName(String const& name) const;
  bool isMaterialName(String const& name) const;
  bool isValidMaterialId(MaterialId material) const;
  MaterialId materialId(String const& materialName) const;
  String materialName(MaterialId materialId) const;
  Maybe<String> materialPath(MaterialId materialId) const;
  Maybe<Json> materialConfig(MaterialId materialId) const;
  String materialDescription(MaterialId materialId, String const& species) const;
  String materialDescription(MaterialId materialId) const;
  String materialShortDescription(MaterialId materialId) const;
  String materialCategory(MaterialId materialId) const;

  StringList modNames() const;
  bool isModName(String const& name) const;
  bool isValidModId(ModId mod) const;
  ModId modId(String const& modName) const;
  String const& modName(ModId modId) const;
  Maybe<String> modPath(ModId modId) const;
  Maybe<Json> modConfig(ModId modId) const;
  String modDescription(ModId modId, String const& species) const;
  String modDescription(ModId modId) const;
  String modShortDescription(ModId modId) const;

  // Will return nullptr if no rendering profile is available
  MaterialRenderProfileConstPtr materialRenderProfile(MaterialId modId) const;
  MaterialRenderProfileConstPtr modRenderProfile(ModId modId) const;

  TileDamageParameters materialDamageParameters(MaterialId materialId) const;
  TileDamageParameters modDamageParameters(ModId modId) const;

  bool modBreaksWithTile(ModId modId) const;

  CollisionKind materialCollisionKind(MaterialId materialId) const;
  bool canPlaceInLayer(MaterialId materialId, TileLayer layer) const;

  // Returned ItemDescriptor may be null
  ItemDescriptor materialItemDrop(MaterialId materialId) const;
  ItemDescriptor modItemDrop(ModId modId) const;

  bool isMultiColor(MaterialId materialId) const;
  bool foregroundLightTransparent(MaterialId materialId) const;
  bool backgroundLightTransparent(MaterialId materialId) const;
  bool occludesBehind(MaterialId materialId) const;

  ParticleConfigPtr miningParticle(MaterialId materialId, ModId modId = NoModId) const;
  String miningSound(MaterialId materialId, ModId modId = NoModId) const;
  String footstepSound(MaterialId materialId, ModId modId = NoModId) const;
  String defaultFootstepSound() const;

  Color materialParticleColor(MaterialId materialId, MaterialHue hueShift) const;
  Vec3F radiantLight(MaterialId materialId, ModId modId) const;

  bool supportsMod(MaterialId materialId, ModId modId) const;
  ModId tilledModFor(MaterialId materialId) const;
  bool isTilledMod(ModId modId) const;

  bool isSoil(MaterialId materialId) const;
  bool isFallingMaterial(MaterialId materialId) const;
  bool isCascadingFallingMaterial(MaterialId materialId) const;
  bool blocksLiquidFlow(MaterialId materialId) const;

  // Returns the amount of liquid to consume, and optionally the material / mod
  // to transform to (may be NullMaterialId / NullModId)
  Maybe<LiquidMaterialInteraction> liquidMaterialInteraction(LiquidId liquid, MaterialId materialId) const;
  Maybe<LiquidModInteraction> liquidModInteraction(LiquidId liquid, ModId modId) const;

private:
  struct MetaMaterialInfo {
    MetaMaterialInfo(String name, MaterialId id, CollisionKind collisionKind, bool blocksLiquidFlow);

    String name;
    MaterialId id;
    CollisionKind collisionKind;
    bool blocksLiquidFlow;
  };

  struct MaterialInfo {
    MaterialInfo();

    String name;
    MaterialId id;
    String path;
    Json config;

    String itemDrop;
    Json descriptions;
    String category;
    Color particleColor;
    ParticleConfigPtr miningParticle;
    StringList miningSounds;
    String footstepSound;
    ModId tillableMod;
    CollisionKind collisionKind;
    bool foregroundOnly;
    bool supportsMods;
    bool soil;
    bool falling;
    bool cascading;
    bool blocksLiquidFlow;

    shared_ptr<MaterialRenderProfile const> materialRenderProfile;

    TileDamageParameters damageParameters;
  };

  struct ModInfo {
    ModInfo();

    String name;
    ModId id;
    String path;
    Json config;

    String itemDrop;
    Json descriptions;
    Color particleColor;
    ParticleConfigPtr miningParticle;
    StringList miningSounds;
    String footstepSound;
    bool tilled;
    bool breaksWithTile;

    shared_ptr<MaterialRenderProfile const> modRenderProfile;

    TileDamageParameters damageParameters;
  };

  size_t metaMaterialIndex(MaterialId materialId) const;
  bool containsMetaMaterial(MaterialId materialId) const;
  void setMetaMaterial(MaterialId materialId, MetaMaterialInfo info);

  bool containsMaterial(MaterialId materialId) const;
  void setMaterial(MaterialId materialId, MaterialInfo info);

  bool containsMod(ModId modId) const;
  void setMod(ModId modId, ModInfo info);

  shared_ptr<MetaMaterialInfo const> const& getMetaMaterialInfo(MaterialId materialId) const;
  shared_ptr<MaterialInfo const> const& getMaterialInfo(MaterialId materialId) const;
  shared_ptr<ModInfo const> const& getModInfo(ModId modId) const;

  List<shared_ptr<MetaMaterialInfo const>> m_metaMaterials;
  StringMap<MaterialId> m_metaMaterialIndex;

  List<shared_ptr<MaterialInfo const>> m_materials;
  StringMap<MaterialId> m_materialIndex;

  List<shared_ptr<ModInfo const>> m_mods;
  StringMap<ModId> m_modIndex;
  BiMap<String, ModId> m_metaModIndex;

  String m_defaultFootstepSound;

  HashMap<pair<LiquidId, MaterialId>, LiquidMaterialInteraction> m_liquidMaterialInteractions;
  HashMap<pair<LiquidId, ModId>, LiquidModInteraction> m_liquidModInteractions;
};

inline MaterialRenderProfileConstPtr MaterialDatabase::materialRenderProfile(MaterialId materialId) const {
  if (materialId < m_materials.size()) {
    if (auto const& mat = m_materials[materialId])
      return mat->materialRenderProfile;
  }
  
  return {};
}

inline MaterialRenderProfileConstPtr MaterialDatabase::modRenderProfile(ModId modId) const {
  if (modId < m_mods.size()) {
    if (auto const& mod = m_mods[modId])
      return mod->modRenderProfile;
  }
  
  return {};
}

inline bool MaterialDatabase::foregroundLightTransparent(MaterialId materialId) const {
  if (isRealMaterial(materialId)) {
    auto const& matInfo = getMaterialInfo(materialId);
    if (matInfo->materialRenderProfile)
      return matInfo->materialRenderProfile->foregroundLightTransparent;
  }

  if (materialId == StructureMaterialId)
    return false;

  return true;
}

inline bool MaterialDatabase::backgroundLightTransparent(MaterialId materialId) const {
  if (isRealMaterial(materialId)) {
    auto const& matInfo = getMaterialInfo(materialId);
    if (matInfo->materialRenderProfile)
      return matInfo->materialRenderProfile->backgroundLightTransparent;
  }

  if (materialId == StructureMaterialId)
    return false;

  return true;
}

inline bool MaterialDatabase::occludesBehind(MaterialId materialId) const {
  if (isRealMaterial(materialId)) {
    auto const& matInfo = getMaterialInfo(materialId);
    if (matInfo->materialRenderProfile)
      return matInfo->materialRenderProfile->occludesBehind;
  }

  return false;
}

inline Vec3F MaterialDatabase::radiantLight(MaterialId materialId, ModId modId) const {
  Vec3F radiantLight;
  if (materialId < m_materials.size()) {
    auto const& mat = m_materials[materialId];
    if (mat && mat->materialRenderProfile)
      radiantLight += mat->materialRenderProfile->radiantLight;
  }
  if (modId < m_mods.size()) {
    auto const& mod = m_mods[modId];
    if (mod && mod->modRenderProfile)
      radiantLight += mod->modRenderProfile->radiantLight;
  }
  return radiantLight;
}

inline bool MaterialDatabase::blocksLiquidFlow(MaterialId materialId) const {
  if (isRealMaterial(materialId))
    return getMaterialInfo(materialId)->blocksLiquidFlow;
  else
    return getMetaMaterialInfo(materialId)->blocksLiquidFlow;

}

inline Maybe<LiquidMaterialInteraction> MaterialDatabase::liquidMaterialInteraction(
    LiquidId liquid, MaterialId materialId) const {
  return m_liquidMaterialInteractions.maybe({liquid, materialId});
}

inline Maybe<LiquidModInteraction> MaterialDatabase::liquidModInteraction(LiquidId liquid, ModId modId) const {
  return m_liquidModInteractions.maybe({liquid, modId});
}
}
