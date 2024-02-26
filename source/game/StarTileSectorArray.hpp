#pragma once

#include "StarRect.hpp"
#include "StarSectorArray2D.hpp"

namespace Star {

// Storage container for world tiles that understands the sector based
// non-euclidean nature of the World.
//
// All RectI regions in this class are assumed to be right/top exclusive, so
// each tile covered by a RectI region must be strictly contained within the
// region to be included.
template <typename TileT, unsigned SectorSizeT>
class TileSectorArray {
public:
  typedef TileT Tile;
  static unsigned const SectorSize = SectorSizeT;

  typedef SectorArray2D<Tile, SectorSize> SectorArray;
  typedef typename SectorArray::Sector Sector;
  typedef typename SectorArray::Array Array;
  typedef typename SectorArray::ArrayPtr ArrayPtr;

  TileSectorArray();
  TileSectorArray(Vec2U const& size, Tile defaultTile = Tile());

  void init(Vec2U const& size, Tile defaultTile = Tile());

  Vec2U size() const;
  Tile defaultTile() const;

  // Returns true if this sector is within the size bounds, regardless of
  // loaded / unloaded status.
  bool sectorValid(Sector const& sector) const;

  Sector sectorFor(Vec2I const& pos) const;

  // Return all valid sectors within a given range, regardless of loaded /
  // unloaded status.
  List<Sector> validSectorsFor(RectI const& region) const;

  // Returns the region for this sector, which is SectorSize x SectorSize
  // large.
  RectI sectorRegion(Sector const& sector) const;

  // Returns adjacent sectors in any given integral movement, in sectors.
  Sector adjacentSector(Sector const& sector, Vec2I const& sectorMovement);

  // Load a sector into the active sector array.
  void loadSector(Sector const& sector, ArrayPtr array);
  // Load with a sector full of the default tile.
  void loadDefaultSector(Sector const& sector);
  // Make a copy of a sector
  ArrayPtr copySector(Sector const& sector);
  // Take a sector out of the sector array.
  ArrayPtr unloadSector(Sector const& sector);

  bool sectorLoaded(Sector sector) const;
  List<Sector> loadedSectors() const;
  size_t loadedSectorCount() const;

  // Will return null if the sector is unloaded.
  Array const* sectorArray(Sector sector) const;
  Array* sectorArray(Sector sector);

  bool tileLoaded(Vec2I const& pos) const;

  Tile const& tile(Vec2I const& pos) const;

  // Will return nullptr if the position is invalid.
  Tile* modifyTile(Vec2I const& pos);

  // Function signature here is (Vec2I const&, Tile const&).  Will be called
  // for the entire region, valid or not.  If tile positions are not valid,
  // they will be called with the defaultTile.
  template <typename Function>
  void tileEach(RectI const& region, Function&& function) const;

  // Behaves like tileEach, but gathers the results of calling the function into
  // a MultiArray
  template <typename Function>
  MultiArray<std::result_of_t<Function(Vec2I, Tile)>, 2> tileEachResult(RectI const& region, Function&& function) const;

  // Fastest way to copy data from the tile array to a given target array.
  // Takes a multi-array and a region and a function, resizes the multi-array
  // to be the size of the given region, and then calls the given function on
  // each tile in the region with this signature:
  // function(MultiArray::Element& target, Vec2I const& position, Tile const& source);
  // Called with the defaultTile for out of range positions.
  template <typename MultiArray, typename Function>
  void tileEachTo(MultiArray& results, RectI const& region, Function&& function) const;

  // Function signature here is (Vec2I const&, Tile&).  If a tile position
  // within this range is not valid, the function *will not* be called for that
  // position.
  template <typename Function>
  void tileEval(RectI const& region, Function&& function);

  // Will not be called for parts of the region that are not valid positions.
  template <typename Function>
  void tileEachColumns(RectI const& region, Function&& function) const;
  template <typename Function>
  void tileEvalColumns(RectI const& region, Function&& function);

  // Searches for a tile that satisfies a given condition in a block-area.
  // Returns true on the first instance found.  Passed in function must accept
  // (Vec2I const&, Tile const&).
  template <typename Function>
  bool tileSatisfies(RectI const& region, Function&& function) const;
  // Same, but uses a radius of 'distance', which is inclusive on all sides.
  // In other words, calling tileSatisfies({0, 0}, 1, <func>) should be
  // equivalent to calling tileSatisfies({-1, -1}, {3, 3}, <func>).
  template <typename Function>
  bool tileSatisfies(Vec2I const& pos, unsigned distance, Function&& function) const;

private:
  struct SplitRect {
    RectI rect;
    int xOffset;
  };

  // function must return bool to continue iteration
  template <typename Function>
  bool tileEachAbortable(RectI const& region, Function&& function) const;

  // Splits rects along the world wrap line and wraps the x coordinate for each
  // rect into world space.  Also returns the integral x offset to transform
  // back into the input rect range.
  StaticList<SplitRect, 2> splitRect(RectI rect) const;

  // Clamp the rect to entirely within valid tile spaces in y dimension
  RectI yClampRect(RectI const& r) const;

  Vec2U m_worldSize;
  Tile m_default;
  SectorArray m_tileSectors;
};

template <typename Tile, unsigned SectorSize>
unsigned const TileSectorArray<Tile, SectorSize>::SectorSize;

template <typename Tile, unsigned SectorSize>
TileSectorArray<Tile, SectorSize>::TileSectorArray() {}

template <typename Tile, unsigned SectorSize>
TileSectorArray<Tile, SectorSize>::TileSectorArray(Vec2U const& size, Tile defaultTile) {
  init(size, std::move(defaultTile));
}

template <typename Tile, unsigned SectorSize>
void TileSectorArray<Tile, SectorSize>::init(Vec2U const& size, Tile defaultTile) {
  m_worldSize = size;
  // Initialize to enough sectors to fit world size at least.
  m_tileSectors.init((size[0] + SectorSize - 1) / SectorSize, (size[1] + SectorSize - 1) / SectorSize);
  m_default = std::move(defaultTile);
}

template <typename Tile, unsigned SectorSize>
Vec2U TileSectorArray<Tile, SectorSize>::size() const {
  return m_worldSize;
}

template <typename Tile, unsigned SectorSize>
Tile TileSectorArray<Tile, SectorSize>::defaultTile() const {
  return m_default;
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::sectorFor(Vec2I const& pos) const -> Sector {
  return m_tileSectors.sectorFor((unsigned)pmod<int>(pos[0], m_worldSize[0]), (unsigned)pos[1]);
}

template <typename Tile, unsigned SectorSize>
bool TileSectorArray<Tile, SectorSize>::sectorValid(Sector const& sector) const {
  return m_tileSectors.sectorValid(sector);
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::validSectorsFor(RectI const& region) const -> List<Sector> {
  List<Sector> sectors;
  for (auto const& split : splitRect(yClampRect(region))) {
    auto sectorRange = m_tileSectors.sectorRange(split.rect.xMin(), split.rect.yMin(), split.rect.width(), split.rect.height());
    sectors.reserve(sectors.size() + (sectorRange.max[0] - sectorRange.min[0]) * (sectorRange.max[1] - sectorRange.min[1]));
    for (size_t x = sectorRange.min[0]; x < sectorRange.max[0]; ++x) {
      for (size_t y = sectorRange.min[1]; y < sectorRange.max[1]; ++y)
        sectors.append({x, y});
    }
  }
  return sectors;
}

template <typename Tile, unsigned SectorSize>
RectI TileSectorArray<Tile, SectorSize>::sectorRegion(Sector const& sector) const {
  Vec2I sectorCorner(m_tileSectors.sectorCorner(sector));
  return RectI::withSize(sectorCorner, {min<int>(SectorSize, m_worldSize[0] - sectorCorner[0]), min<int>(SectorSize, m_worldSize[1] - sectorCorner[1])});
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::adjacentSector(Sector const& sector, Vec2I const& sectorMovement) -> Sector {
  // This works because the only smaller than SectorSize sectors are on the
  // world wrap point, and there is only one vertical line of them, but it's
  // very not-obvious that it works.
  Vec2I corner(m_tileSectors.sectorCorner(sector));
  corner += sectorMovement * SectorSize;
  return sectorFor(corner);
}

template <typename Tile, unsigned SectorSize>
void TileSectorArray<Tile, SectorSize>::loadSector(Sector const& sector, ArrayPtr tile) {
  if (sectorValid(sector))
    m_tileSectors.loadSector(sector, std::move(tile));
}

template <typename Tile, unsigned SectorSize>
void TileSectorArray<Tile, SectorSize>::loadDefaultSector(Sector const& sector) {
  if (sectorValid(sector))
    m_tileSectors.loadSector(sector, std::make_unique<Array>(m_default));
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::copySector(Sector const& sector) -> ArrayPtr {
  if (sectorValid(sector))
    return m_tileSectors.copySector(sector);
  else
    return {};
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::unloadSector(Sector const& sector) -> ArrayPtr {
  if (sectorValid(sector))
    return m_tileSectors.takeSector(sector);
  else
    return {};
}

template <typename Tile, unsigned SectorSize>
bool TileSectorArray<Tile, SectorSize>::sectorLoaded(Sector sector) const {
  if (sectorValid(sector))
    return m_tileSectors.sectorLoaded(sector);
  else
    return false;
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::loadedSectors() const -> List<Sector> {
  return m_tileSectors.loadedSectors();
}

template <typename Tile, unsigned SectorSize>
size_t TileSectorArray<Tile, SectorSize>::loadedSectorCount() const {
  return m_tileSectors.loadedSectorCount();
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::sectorArray(Sector sector) const -> Array const * {
  if (sectorValid(sector))
    return m_tileSectors.sector(sector);
  else
    return nullptr;
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::sectorArray(Sector sector) -> Array * {
  if (sectorValid(sector))
    return m_tileSectors.sector(sector);
  else
    return nullptr;
}

template <typename Tile, unsigned SectorSize>
bool TileSectorArray<Tile, SectorSize>::tileLoaded(Vec2I const& pos) const {
  if (pos[1] < 0 || pos[1] >= (int)m_worldSize[1])
    return false;

  unsigned xind = (unsigned)pmod<int>(pos[0], m_worldSize[0]);
  unsigned yind = (unsigned)pos[1];

  return m_tileSectors.get(xind, yind) != nullptr;
}

template <typename Tile, unsigned SectorSize>
Tile const& TileSectorArray<Tile, SectorSize>::tile(Vec2I const& pos) const {
  if (pos[1] < 0 || pos[1] >= (int)m_worldSize[1])
    return m_default;

  unsigned xind = (unsigned)pmod<int>(pos[0], m_worldSize[0]);
  unsigned yind = (unsigned)pos[1];

  Tile const* tile = m_tileSectors.get(xind, yind);
  if (tile)
    return *tile;
  else
    return m_default;
}

template <typename Tile, unsigned SectorSize>
Tile* TileSectorArray<Tile, SectorSize>::modifyTile(Vec2I const& pos) {
  if (pos[1] < 0 || pos[1] >= (int)m_worldSize[1])
    return nullptr;

  unsigned xind = (unsigned)pmod<int>(pos[0], m_worldSize[0]);
  unsigned yind = (unsigned)pos[1];

  return m_tileSectors.get(xind, yind);
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
void TileSectorArray<Tile, SectorSize>::tileEach(RectI const& region, Function&& function) const {
  tileEachAbortable(region,
      [&](Vec2I const& pos, Tile const& tile) {
        function(pos, tile);
        return true;
      });
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
MultiArray<std::result_of_t<Function(Vec2I, Tile)>, 2> TileSectorArray<Tile, SectorSize>::tileEachResult(RectI const& region, Function&& function) const {
  MultiArray<std::result_of_t<Function(Vec2I, Tile)>, 2> res;
  tileEachTo(res, region, [&](auto& res, Vec2I const& pos, Tile const& tile) { res = function(pos, tile); });
  return res;
}

template <typename Tile, unsigned SectorSize>
template <typename MultiArray, typename Function>
void TileSectorArray<Tile, SectorSize>::tileEachTo(MultiArray& results, RectI const& region, Function&& function) const {
  if (region.isEmpty()) {
    results.setSize({0, 0});
    return;
  }

  int xArrayOffset = -region.xMin();
  int yArrayOffset = -region.yMin();
  results.setSize(Vec2S(region.size()));

  for (auto const& split : splitRect(region)) {
    auto clampedRect = yClampRect(split.rect);
    if (!clampedRect.isEmpty()) {
      m_tileSectors.evalColumns(clampedRect.xMin(), clampedRect.yMin(), clampedRect.width(), clampedRect.height(), [&](size_t x, size_t y, Tile const* column, size_t columnSize) {
          size_t arrayColumnIndex = (x + split.xOffset + xArrayOffset) * results.size(1) + y + yArrayOffset;
          if (column) {
            for (size_t i = 0; i < columnSize; ++i)
              function(results.atIndex(arrayColumnIndex + i), Vec2I((int)x + split.xOffset, y + i), column[i]);
          } else {
            for (size_t i = 0; i < columnSize; ++i)
              function(results.atIndex(arrayColumnIndex + i), Vec2I((int)x + split.xOffset, y + i), m_default);
          }
          return true;
        }, true);
    }

    // Call with default tile for tiles outside of the y-range (to ensure that
    // every index in the rect gets called)
    for (int x = split.rect.xMin(); x < split.rect.xMax(); ++x) {
      for (int y = split.rect.yMin(); y < min<int>(split.rect.yMax(), 0); ++y)
        function(results(x + split.xOffset + xArrayOffset, y + yArrayOffset), Vec2I(x + split.xOffset, y), m_default);
    }

    for (int x = split.rect.xMin(); x < split.rect.xMax(); ++x) {
      for (int y = max<int>(split.rect.yMin(), m_worldSize[1]); y < split.rect.yMax(); ++y)
        function(results(x + split.xOffset + xArrayOffset, y + yArrayOffset), Vec2I(x + split.xOffset, y), m_default);
    }
  }
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
void TileSectorArray<Tile, SectorSize>::tileEval(RectI const& region, Function&& function) {
  for (auto const& split : splitRect(region)) {
    auto clampedRect = yClampRect(split.rect);
    if (!clampedRect.isEmpty()) {
      // If non-const variant, do not call function if tile not loaded (pass
      // false to evalEmpty in sector array)
      auto fwrapper = [&](unsigned x, unsigned y, Tile* tile) {
        function(Vec2I((int)x + split.xOffset, (int)y), *tile);
        return true;
      };
      m_tileSectors.eval(clampedRect.xMin(), clampedRect.yMin(), clampedRect.width(), clampedRect.height(), fwrapper, false);
    }
  }
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
void TileSectorArray<Tile, SectorSize>::tileEachColumns(RectI const& region, Function&& function) const {
  const_cast<TileSectorArray*>(this)->tileEvalColumns(
      region, [&](Vec2I const& pos, Tile* tiles, size_t size) { function(pos, (Tile const*)tiles, size); });
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
void TileSectorArray<Tile, SectorSize>::tileEvalColumns(RectI const& region, Function&& function) {
  for (auto const& split : splitRect(region)) {
    auto clampedRect = yClampRect(split.rect);
    if (!clampedRect.isEmpty()) {
      auto fwrapper = [&](size_t x, size_t y, Tile* column, size_t columnSize) {
        function(Vec2I((int)x + split.xOffset, (int)y), column, columnSize);
        return true;
      };
      m_tileSectors.evalColumns(clampedRect.xMin(), clampedRect.yMin(), clampedRect.width(), clampedRect.height(), fwrapper, false);
    }
  }
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
bool TileSectorArray<Tile, SectorSize>::tileSatisfies(Vec2I const& pos, unsigned distance, Function&& function) const {
  return tileSatisfies(RectI::withSize(pos - Vec2I::filled(distance), Vec2I::filled(distance * 2 + 1)), function);
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
bool TileSectorArray<Tile, SectorSize>::tileSatisfies(RectI const& region, Function&& function) const {
  return !tileEachAbortable(region, [&](Vec2I const& pos, Tile const& tile) { return !function(pos, tile); });
}

template <typename Tile, unsigned SectorSize>
template <typename Function>
bool TileSectorArray<Tile, SectorSize>::tileEachAbortable(RectI const& region, Function&& function) const {
  for (auto const& split : splitRect(region)) {
    auto clampedRect = yClampRect(split.rect);
    if (!clampedRect.isEmpty()) {
      // If const variant, call function with default tile if not loaded.
      auto fwrapper = [&](unsigned x, unsigned y, Tile const* tile) {
        if (!tile)
          tile = &m_default;
        return function(Vec2I((int)x + split.xOffset, y), *tile);
      };
      if (!m_tileSectors.eval(clampedRect.xMin(), clampedRect.yMin(), clampedRect.width(), clampedRect.height(), fwrapper, true))
        return false;
    }

    // Call with default tile for tiles outside of the y-range (to ensure
    // that every index in the rect gets called)
    for (int x = split.rect.xMin(); x < split.rect.xMax(); ++x) {
      for (int y = split.rect.yMin(); y < min<int>(split.rect.yMax(), 0); ++y) {
        if (!function(Vec2I(x + split.xOffset, y), m_default))
          return false;
      }
    }

    for (int x = split.rect.xMin(); x < split.rect.xMax(); ++x) {
      for (int y = max<int>(split.rect.yMin(), m_worldSize[1]); y < split.rect.yMax(); ++y)
        if (!function(Vec2I(x + split.xOffset, y), m_default))
          return false;
    }
  }
  return true;
}

template <typename Tile, unsigned SectorSize>
auto TileSectorArray<Tile, SectorSize>::splitRect(RectI rect) const -> StaticList<SplitRect, 2> {
  // TODO: Offset here does not support rects outside of -m_worldSize[0] to 2 * m_worldSize[0]!
  starAssert(rect.xMin() >= -(int)m_worldSize[0] && rect.xMax() <= 2 * (int)m_worldSize[0]);

  // any rect at least the width of the world is equivalent to a rect that spans the width of the world exactly
  if (rect.width() >= (int)m_worldSize[0])
    return{SplitRect{RectI(0, rect.yMin(), m_worldSize[0], rect.yMax()), 0}};

  if (rect.isEmpty())
    return {};

  int width = rect.width();
  int xMin = pmod<int>(rect.xMin(), m_worldSize[0]);
  int xOffset = rect.xMin() - xMin;
  rect.setXMin(xMin);
  rect.setXMax(xMin + width);

  if (rect.xMin() < (int)m_worldSize[0] && rect.xMax() > (int)m_worldSize[0]) {
    return {
      SplitRect{RectI(rect.xMin(), rect.yMin(), m_worldSize[0], rect.yMax()), xOffset},
      SplitRect{RectI(0, rect.yMin(), rect.xMax() - m_worldSize[0], rect.yMax()), xOffset + (int)m_worldSize[0]}
    };
  } else {
    return {SplitRect{rect, xOffset}};
  }
}

template <typename Tile, unsigned SectorSize>
RectI TileSectorArray<Tile, SectorSize>::yClampRect(RectI const& r) const {
  return RectI(r.xMin(), clamp<int>(r.yMin(), 0, m_worldSize[1]), r.xMax(), clamp<int>(r.yMax(), 0, m_worldSize[1]));
}

}
