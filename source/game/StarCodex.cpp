#include "StarCodex.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

Codex::Codex(Json const& config, String const& directory) {
  m_directory = directory;
  m_id = config.getString("id");
  m_species = config.getString("species", "other");
  m_title = config.getString("title");
  m_description = config.getString("description", "");
  m_icon = config.getString("icon");
  m_pages = jsonToStringList(config.get("contentPages"));
  m_itemConfig = config.get("itemConfig", Json());
}

Json Codex::toJson() const {
  auto result = JsonObject{
      {"id", m_id},
      {"species", m_species},
      {"title", m_title},
      {"description", m_description},
      {"icon", m_icon},
      {"contentPages", jsonFromStringList(m_pages)},
      {"itemConfig", m_itemConfig}};
  return result;
}

String Codex::id() const {
  return m_id;
}

String Codex::species() const {
  return m_species;
}

String Codex::title() const {
  return m_title;
}

String Codex::description() const {
  return m_description;
}

String Codex::icon() const {
  return m_icon;
}

String Codex::page(size_t pageNum) const {
  if (pageNum < m_pages.size())
    return m_pages[pageNum];
  return "";
}

List<String> Codex::pages() const {
  return m_pages;
}

size_t Codex::pageCount() const {
  return m_pages.size();
}

Json Codex::itemConfig() const {
  return m_itemConfig;
}

String Codex::directory() const {
  return m_directory;
}

}
