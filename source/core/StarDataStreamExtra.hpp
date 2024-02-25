#pragma once

#include "StarDataStream.hpp"
#include "StarMultiArray.hpp"
#include "StarColor.hpp"
#include "StarPoly.hpp"
#include "StarMaybe.hpp"
#include "StarEither.hpp"
#include "StarOrderedMap.hpp"
#include "StarOrderedSet.hpp"

namespace Star {

struct DataStreamWriteFunctor {
  DataStreamWriteFunctor(DataStream& ds) : ds(ds) {}

  DataStream& ds;
  template <typename T>
  void operator()(T const& t) const {
    ds << t;
  }
};

struct DataStreamReadFunctor {
  DataStreamReadFunctor(DataStream& ds) : ds(ds) {}

  DataStream& ds;
  template <typename T>
  void operator()(T& t) const {
    ds >> t;
  }
};

inline DataStream& operator<<(DataStream& ds, Empty const&) {
  return ds;
}

inline DataStream& operator>>(DataStream& ds, Empty&) {
  return ds;
}

template <typename ElementT, size_t SizeN>
DataStream& operator<<(DataStream& ds, Array<ElementT, SizeN> const& array) {
  for (size_t i = 0; i < SizeN; ++i)
    ds << array[i];
  return ds;
}

template <typename ElementT, size_t SizeN>
DataStream& operator>>(DataStream& ds, Array<ElementT, SizeN>& array) {
  for (size_t i = 0; i < SizeN; ++i)
    ds >> array[i];
  return ds;
}

template <typename ElementT, size_t RankN>
DataStream& operator<<(DataStream& ds, MultiArray<ElementT, RankN> const& array) {
  auto size = array.size();
  for (size_t i = 0; i < RankN; ++i)
    ds.writeVlqU(size[i]);

  size_t count = array.count();
  for (size_t i = 0; i < count; ++i)
    ds << array.atIndex(i);

  return ds;
}

template <typename ElementT, size_t RankN>
DataStream& operator>>(DataStream& ds, MultiArray<ElementT, RankN>& array) {
  typename MultiArray<ElementT, RankN>::SizeList size;
  for (size_t i = 0; i < RankN; ++i)
    size[i] = ds.readVlqU();

  array.setSize(size);
  size_t count = array.count();
  for (size_t i = 0; i < count; ++i)
    ds >> array.atIndex(i);

  return ds;
}

inline DataStream& operator<<(DataStream& ds, Color const& color) {
  ds << color.toRgbaF();
  return ds;
}

inline DataStream& operator>>(DataStream& ds, Color& color) {
  color = Color::rgbaf(ds.read<Vec4F>());
  return ds;
}

template <typename First, typename Second>
DataStream& operator<<(DataStream& ds, pair<First, Second> const& pair) {
  ds << pair.first;
  ds << pair.second;
  return ds;
}

template <typename First, typename Second>
DataStream& operator>>(DataStream& ds, pair<First, Second>& pair) {
  ds >> pair.first;
  ds >> pair.second;
  return ds;
}

template <typename Element>
DataStream& operator<<(DataStream& ds, std::shared_ptr<Element> const& ptr) {
  ds.pwrite(ptr);
  return ds;
}

template <typename Element>
DataStream& operator>>(DataStream& ds, std::shared_ptr<Element>& ptr) {
  ds.pread(ptr);
  return ds;
}

template <typename BaseList>
DataStream& operator<<(DataStream& ds, ListMixin<BaseList> const& list) {
  ds.writeContainer(list);
  return ds;
}

template <typename BaseList>
DataStream& operator>>(DataStream& ds, ListMixin<BaseList>& list) {
  ds.readContainer(list);
  return ds;
}

template <typename BaseSet>
DataStream& operator<<(DataStream& ds, SetMixin<BaseSet> const& set) {
  ds.writeContainer(set);
  return ds;
}

template <typename BaseSet>
DataStream& operator>>(DataStream& ds, SetMixin<BaseSet>& set) {
  ds.readContainer(set);
  return ds;
}

template <typename BaseMap>
DataStream& operator<<(DataStream& ds, MapMixin<BaseMap> const& map) {
  ds.writeMapContainer(map);
  return ds;
}

template <typename BaseMap>
DataStream& operator>>(DataStream& ds, MapMixin<BaseMap>& map) {
  ds.readMapContainer(map);
  return ds;
}

template <typename Key, typename Value, typename Compare, typename Allocator>
DataStream& operator>>(DataStream& ds, OrderedMap<Key, Value, Compare, Allocator>& map) {
  ds.readMapContainer(map);
  return ds;
}

template <typename Key, typename Value, typename Compare, typename Allocator>
DataStream& operator<<(DataStream& ds, OrderedMap<Key, Value, Compare, Allocator> const& map) {
  ds.writeMapContainer(map);
  return ds;
}

template <typename Key, typename Value, typename Hash, typename Equals, typename Allocator>
DataStream& operator>>(DataStream& ds, OrderedHashMap<Key, Value, Hash, Equals, Allocator>& map) {
  ds.readMapContainer(map);
  return ds;
}

template <typename Key, typename Value, typename Hash, typename Equals, typename Allocator>
DataStream& operator<<(DataStream& ds, OrderedHashMap<Key, Value, Hash, Equals, Allocator> const& map) {
  ds.writeMapContainer(map);
  return ds;
}

template <typename Value, typename Compare, typename Allocator>
DataStream& operator>>(DataStream& ds, OrderedSet<Value, Compare, Allocator>& set) {
  ds.readContainer(set);
  return ds;
}

template <typename Value, typename Compare, typename Allocator>
DataStream& operator<<(DataStream& ds, OrderedSet<Value, Compare, Allocator> const& set) {
  ds.writeContainer(set);
  return ds;
}

template <typename Value, typename Hash, typename Equals, typename Allocator>
DataStream& operator>>(DataStream& ds, OrderedHashSet<Value, Hash, Equals, Allocator>& set) {
  ds.readContainer(set);
  return ds;
}

template <typename Value, typename Hash, typename Equals, typename Allocator>
DataStream& operator<<(DataStream& ds, OrderedHashSet<Value, Hash, Equals, Allocator> const& set) {
  ds.writeContainer(set);
  return ds;
}

template <typename DataT>
DataStream& operator<<(DataStream& ds, Polygon<DataT> const& poly) {
  ds.writeContainer(poly.vertexes());
  return ds;
}

template <typename DataT>
DataStream& operator>>(DataStream& ds, Polygon<DataT>& poly) {
  ds.readContainer(poly.vertexes());
  return ds;
}

template <typename DataT, size_t Dimensions>
DataStream& operator<<(DataStream& ds, Box<DataT, Dimensions> const& box) {
  ds.write(box.min());
  ds.write(box.max());
  return ds;
}

template <typename DataT, size_t Dimensions>
DataStream& operator>>(DataStream& ds, Box<DataT, Dimensions>& box) {
  ds.read(box.min());
  ds.read(box.max());
  return ds;
}

template <typename DataT>
DataStream& operator<<(DataStream& ds, Matrix3<DataT> const& mat3) {
  ds.write(mat3[0]);
  ds.write(mat3[1]);
  ds.write(mat3[2]);
  return ds;
}

template <typename DataT>
DataStream& operator>>(DataStream& ds, Matrix3<DataT>& mat3) {
  ds.read(mat3[0]);
  ds.read(mat3[1]);
  ds.read(mat3[2]);
  return ds;
}

// Writes / reads a Variant type if every type has operator<< / operator>>
// defined for DataStream and if it is default constructible.

template <typename FirstType, typename... RestTypes>
DataStream& operator<<(DataStream& ds, Variant<FirstType, RestTypes...> const& variant) {
  ds.write<VariantTypeIndex>(variant.typeIndex());
  variant.call(DataStreamWriteFunctor{ds});
  return ds;
}

template <typename FirstType, typename... RestTypes>
DataStream& operator>>(DataStream& ds, Variant<FirstType, RestTypes...>& variant) {
  variant.makeType(ds.read<VariantTypeIndex>());
  variant.call(DataStreamReadFunctor{ds});
  return ds;
}

template <typename... AllowedTypes>
DataStream& operator<<(DataStream& ds, MVariant<AllowedTypes...> const& mvariant) {
  ds.write<VariantTypeIndex>(mvariant.typeIndex());
  mvariant.call(DataStreamWriteFunctor{ds});
  return ds;
}

template <typename... AllowedTypes>
DataStream& operator>>(DataStream& ds, MVariant<AllowedTypes...>& mvariant) {
  mvariant.makeType(ds.read<VariantTypeIndex>());
  mvariant.call(DataStreamReadFunctor{ds});
  return ds;
}

// Writes / reads a Maybe type if the underlying type has operator<< /
// operator>> defined for DataStream

template <typename T, typename WriteFunction>
void writeMaybe(DataStream& ds, Maybe<T> const& maybe, WriteFunction&& writeFunction) {
  if (maybe) {
    ds.write<bool>(true);
    writeFunction(ds, *maybe);
  } else {
    ds.write<bool>(false);
  }
}

template <typename T, typename ReadFunction>
void readMaybe(DataStream& ds, Maybe<T>& maybe, ReadFunction&& readFunction) {
  bool set = ds.read<bool>();
  if (set) {
    T t;
    readFunction(ds, t);
    maybe = std::move(t);
  } else {
    maybe.reset();
  }
}

template <typename T>
DataStream& operator<<(DataStream& ds, Maybe<T> const& maybe) {
  writeMaybe(ds, maybe, [](DataStream& ds, T const& t) { ds << t; });
  return ds;
}

template <typename T>
DataStream& operator>>(DataStream& ds, Maybe<T>& maybe) {
  readMaybe(ds, maybe, [](DataStream& ds, T& t) { ds >> t; });
  return ds;
}

// Writes / reads an Either type, an Either can either have a left or right
// value, or in edge cases, nothing.

template <typename Left, typename Right>
DataStream& operator<<(DataStream& ds, Either<Left, Right> const& either) {
  if (either.isLeft()) {
    ds.write<uint8_t>(1);
    ds.write(either.left());
  } else if (either.isRight()) {
    ds.write<uint8_t>(2);
    ds.write(either.right());
  } else {
    ds.write<uint8_t>(0);
  }
  return ds;
}

template <typename Left, typename Right>
DataStream& operator>>(DataStream& ds, Either<Left, Right>& either) {
  uint8_t m = ds.read<uint8_t>();
  if (m == 1)
    either = makeLeft(ds.read<Left>());
  else if (m == 2)
    either = makeRight(ds.read<Right>());
  return ds;
}

template <typename DataT, size_t Dimensions>
DataStream& operator<<(DataStream& ds, Line<DataT, Dimensions> const& line) {
  ds.write(line.min());
  ds.write(line.max());
  return ds;
}

template <typename DataT, size_t Dimensions>
DataStream& operator>>(DataStream& ds, Line<DataT, Dimensions>& line) {
  ds.read(line.min());
  ds.read(line.max());
  return ds;
}

template <typename T>
DataStream& operator<<(DataStream& ds, tuple<T> const& t) {
  ds << get<0>(t);
  return ds;
}

struct DataStreamReader {
  DataStream& ds;

  template <typename RT>
  void operator()(RT& t) {
    ds >> t;
  }
};

struct DataStreamWriter {
  DataStream& ds;

  template <typename RT>
  void operator()(RT const& t) {
    ds << t;
  }
};

template <typename... T>
DataStream& operator>>(DataStream& ds, tuple<T...>& t) {
  tupleCallFunction(t, DataStreamReader{ds});
  return ds;
}

template <typename... T>
DataStream& operator<<(DataStream& ds, tuple<T...> const& t) {
  tupleCallFunction(t, DataStreamWriter{ds});
  return ds;
}

}
