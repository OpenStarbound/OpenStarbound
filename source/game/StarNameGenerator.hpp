#ifndef STAR_NAME_GENERATOR_HPP
#define STAR_NAME_GENERATOR_HPP

#include "StarJson.hpp"
#include "StarRandom.hpp"

namespace Star {

STAR_EXCEPTION(NameGeneratorException, StarException);

STAR_CLASS(PatternedNameGenerator);

struct MarkovSource {
  size_t prefixSize;
  size_t endSize;
  StringList starts;
  StringMap<StringList> chains;
  StringSet ends;
};

class PatternedNameGenerator {
public:
  PatternedNameGenerator();

  String generateName(String const& rulesAsset) const;
  String generateName(String const& rulesAsset, uint64_t seed) const;
  String generateName(String const& rulesAsset, RandomSource& random) const;

private:
  String processRule(JsonArray const& rule, RandomSource& random) const;

  bool isProfane(String const& name) const;

  MarkovSource makeMarkovSource(size_t prefixSize, size_t endSize, StringList sourceNames);

  StringMap<MarkovSource> m_markovSources;
  StringSet m_profanityFilter;
};

}

#endif
