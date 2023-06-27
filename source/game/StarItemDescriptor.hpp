#ifndef STAR_ITEM_DESCRIPTOR_HPP
#define STAR_ITEM_DESCRIPTOR_HPP

#include "StarJson.hpp"
#include "StarDataStream.hpp"
#include "StarBiMap.hpp"

namespace Star {

STAR_CLASS(ItemDescriptor);
STAR_CLASS(Item);
STAR_STRUCT(VersionedJson);

class ItemDescriptor {
public:
  // Loads ItemDescriptor from store format.
  static ItemDescriptor loadStore(Json const& store);

  ItemDescriptor();
  ItemDescriptor(String name, uint64_t count, Json parameters = Json());

  // Populate from a configuration JsonArray containing up to 3 elements, the
  // name, count, and then any item parameters.  If the json is a map, looks
  // for keys 'name', 'parameters', and 'count'.
  explicit ItemDescriptor(Json const& spec);

  String const& name() const;
  uint64_t count() const;
  Json const& parameters() const;

  ItemDescriptor singular() const;
  ItemDescriptor withCount(uint64_t count) const;
  ItemDescriptor multiply(uint64_t count) const;
  ItemDescriptor applyParameters(JsonObject const& parameters) const;

  // Descriptor is the default constructed ItemDescriptor()
  bool isNull() const;

  // Descriptor is not null
  explicit operator bool() const;

  // True if descriptor is null OR if descriptor is size 0
  bool isEmpty() const;

  bool operator==(ItemDescriptor const& rhs) const;
  bool operator!=(ItemDescriptor const& rhs) const;

  bool matches(ItemDescriptor const& other, bool exactMatch = false) const;
  bool matches(ItemConstPtr const& other, bool exactMatch = false) const;

  // Stores ItemDescriptor to versioned structure not meant for human reading / writing.
  Json diskStore() const;

  // Converts ItemDescriptor to spec format
  Json toJson() const;

  friend DataStream& operator>>(DataStream& ds, ItemDescriptor& itemDescriptor);
  friend DataStream& operator<<(DataStream& ds, ItemDescriptor const& itemDescriptor);

  friend std::ostream& operator<<(std::ostream& os, ItemDescriptor const& descriptor);

  friend struct hash<ItemDescriptor>;

private:
  ItemDescriptor(String name, uint64_t count, Json parameters, Maybe<size_t> parametersHash);

  size_t parametersHash() const;

  String m_name;
  uint64_t m_count;
  Json m_parameters;
  mutable Maybe<size_t> m_parametersHash;
};

template <>
struct hash<ItemDescriptor> {
  size_t operator()(ItemDescriptor const& v) const;
};

}

template <> struct fmt::formatter<Star::ItemDescriptor> : ostream_formatter {};

#endif
