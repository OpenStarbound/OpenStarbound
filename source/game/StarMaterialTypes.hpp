#ifndef STAR_MATERIAL_TYPES_HPP
#define STAR_MATERIAL_TYPES_HPP

namespace Star {

typedef uint16_t MaterialId;
typedef uint8_t MaterialHue;

// Empty and non-colliding.
MaterialId const EmptyMaterialId = 65535;

// Empty and colliding. Also used as a placeholder in world generation, and to
// signal a block which has not yet been loaded
MaterialId const NullMaterialId = 65534;

// Invisible colliding material used for pre-drawn world strucutres.
MaterialId const StructureMaterialId = 65533;

// Placeholder material used in dungeon generation for biome native ground
// material.
MaterialId const Biome5MaterialId = 65532;
MaterialId const Biome4MaterialId = 65531;
MaterialId const Biome3MaterialId = 65530;
MaterialId const Biome2MaterialId = 65529;
MaterialId const Biome1MaterialId = 65528;
MaterialId const BiomeMaterialId = 65527;

// invisible walls that can't be connected to or grappled
MaterialId const BoundaryMaterialId = 65526;

// default generic object metamaterials
MaterialId const ObjectSolidMaterialId = 65500;
MaterialId const ObjectPlatformMaterialId = 65501;

// Material IDs 65500 and above are reserved for engine-specified metamaterials
MaterialId const FirstEngineMetaMaterialId = 65500;

// Material IDs 65000 - 65499 are reserved for configurable metamaterials
// to be used by tile entities or scripts
MaterialId const FirstMetaMaterialId = 65000;

typedef uint8_t MaterialColorVariant;
MaterialColorVariant const DefaultMaterialColorVariant = 0;
MaterialColorVariant const MaxMaterialColorVariant = 8;

typedef uint16_t ModId;

// Tile has no tilemod
ModId const NoModId = 65535;

// Placeholder mod used in dungeon generation for biome native ground mod.
ModId const BiomeModId = 65534;
ModId const UndergroundBiomeModId = 65533;

// The first mod id that is reserved for special hard-coded mod values.
ModId const FirstMetaMod = 65520;

float materialHueToDegrees(MaterialHue hue);
MaterialHue materialHueFromDegrees(float degrees);

bool isRealMaterial(MaterialId material);
bool isConnectableMaterial(MaterialId material);
bool isBiomeMaterial(MaterialId material);

bool isRealMod(ModId mod);
bool isBiomeMod(ModId mod);

inline float materialHueToDegrees(MaterialHue hue) {
  return hue * 360.0f / 255.0f;
}

inline MaterialHue materialHueFromDegrees(float degrees) {
  return (MaterialHue)(fmod(degrees, 360.0f) * 255.0f / 360.0f);
}

inline bool isRealMaterial(MaterialId material) {
  return material < FirstMetaMaterialId;
}

// TODO: metamaterials need more flexibility to define whether they're connectable,
// but this is used in several performance intensive areas, some of which don't
// and probably shouldn't use the material database
inline bool isConnectableMaterial(MaterialId material) {
  return !(material == NullMaterialId || material == EmptyMaterialId || material == BoundaryMaterialId);
}

inline bool isBiomeMaterial(MaterialId material) {
  return (material == BiomeMaterialId) || ((material >= Biome1MaterialId) && (material <= Biome5MaterialId));
}

inline bool isRealMod(ModId mod) {
  return mod < FirstMetaMod;
}

inline bool isBiomeMod(ModId mod) {
  return mod == BiomeModId || mod == UndergroundBiomeModId;
}
}

#endif
