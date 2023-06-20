#ifndef STAR_UUID_HPP
#define STAR_UUID_HPP

#include "StarArray.hpp"
#include "StarDataStream.hpp"

namespace Star {

STAR_EXCEPTION(UuidException, StarException);

size_t const UuidSize = 16;

class Uuid {
public:
  Uuid();
  explicit Uuid(ByteArray const& bytes);
  explicit Uuid(String const& hex);

  char const* ptr() const;
  ByteArray bytes() const;
  String hex() const;

  bool operator==(Uuid const& u) const;
  bool operator!=(Uuid const& u) const;
  bool operator<(Uuid const& u) const;
  bool operator<=(Uuid const& u) const;
  bool operator>(Uuid const& u) const;
  bool operator>=(Uuid const& u) const;

private:
  Array<char, UuidSize> m_data;
};

template <>
struct hash<Uuid> {
  size_t operator()(Uuid const& u) const;
};

DataStream& operator>>(DataStream& ds, Uuid& uuid);
DataStream& operator<<(DataStream& ds, Uuid const& uuid);

}

#endif
