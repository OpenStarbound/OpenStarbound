#ifndef STAR_CODEX_DATABASE_HPP
#define STAR_CODEX_DATABASE_HPP

#include "StarJson.hpp"
#include "StarCodex.hpp"

namespace Star {

STAR_EXCEPTION(CodexDatabaseException, StarException);

STAR_CLASS(CodexDatabase);

class CodexDatabase {
public:
  CodexDatabase();

  StringMap<CodexConstPtr> codexes() const;
  CodexConstPtr codex(String const& codexId) const;

private:
  StringMap<CodexConstPtr> m_codexes;
};

}

#endif
