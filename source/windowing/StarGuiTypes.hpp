#pragma once

#include "StarString.hpp"
#include "StarVector.hpp"
#include "StarBiMap.hpp"
#include "StarGameTypes.hpp"

namespace Star {

struct ImageStretchSet {
  // how does inner fill up space?
  enum class ImageStretchType {
    Stretch,
    Repeat
  };

  String begin;
  String inner;
  String end;
  ImageStretchType type;

  bool fullyPopulated() const;
};

enum class GuiDirection {
  Horizontal,
  Vertical
};
extern EnumMap<GuiDirection> const GuiDirectionNames;

GuiDirection otherDirection(GuiDirection direction);

template <typename T>
T directionalValueFromVector(GuiDirection direction, Vector<T, 2> const& vec);

String rarityBorder(Rarity rarity);

template <typename T>
T& directionalValueFromVector(GuiDirection direction, Vector<T, 2> const& vec) {
  switch (direction) {
    case GuiDirection::Horizontal:
      return vec[0];
    case GuiDirection::Vertical:
      return vec[1];
  }
}

}
