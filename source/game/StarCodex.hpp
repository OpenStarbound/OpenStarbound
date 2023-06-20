#ifndef STAR_CODEX_HPP
#define STAR_CODEX_HPP

#include "StarJson.hpp"

namespace Star {

STAR_CLASS(Codex);

class Codex {
public:
  Codex(Json const& config, String const& directory);
  Json toJson() const;

  String id() const;
  String species() const;
  String title() const;
  String description() const;
  String icon() const;
  String page(size_t pageNum) const;
  List<String> pages() const;
  size_t pageCount() const;
  Json itemConfig() const;
  String directory() const;

private:
  String m_id;
  String m_species;
  String m_title;
  String m_description;
  String m_icon;
  List<String> m_pages;
  Json m_itemConfig;
  String m_directory;
};

}

#endif
