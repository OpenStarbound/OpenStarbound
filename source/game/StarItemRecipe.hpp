#ifndef STAR_ITEM_RECIPE_HPP
#define STAR_ITEM_RECIPE_HPP

#include "StarItemDescriptor.hpp"
#include "StarGameTypes.hpp"

namespace Star {

STAR_EXCEPTION(RecipeException, StarException);

struct ItemRecipe {
  Json toJson();

  bool isNull();

  bool operator==(ItemRecipe const& rhs) const;
  bool operator!=(ItemRecipe const& rhs) const;

  StringMap<uint64_t> currencyInputs;
  List<ItemDescriptor> inputs;
  ItemDescriptor output;
  float duration;
  StringSet groups;
  Rarity outputRarity;
  String guiFilterString;
  StringMap<String> collectables;
  bool matchInputParameters;
};

template <>
struct hash<ItemRecipe> {
  size_t operator()(ItemRecipe const& v) const;
};

std::ostream& operator<<(std::ostream& os, ItemRecipe const& recipe);
}

template <> struct fmt::formatter<Star::ItemRecipe> : ostream_formatter {};

#endif
