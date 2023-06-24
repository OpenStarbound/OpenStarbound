#include "StarAssetPath.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

// The filename is everything after the last slash (excluding directives) and
// up to the first directive marker.
static Maybe<pair<size_t, size_t>> findFilenameRange(std::string const& pathUtf8) {
  size_t firstDirectiveOrSubPath = pathUtf8.find_first_of(":?");
  size_t filenameStart = 0;
  while (true) {
    size_t find = pathUtf8.find('/', filenameStart);
    if (find >= firstDirectiveOrSubPath)
      break;
    filenameStart = find + 1;
  }

  if (filenameStart == NPos) {
    return {};
  } else if (firstDirectiveOrSubPath == NPos) {
    return {{filenameStart, pathUtf8.size()}};
  } else {
    return {{filenameStart, firstDirectiveOrSubPath}};
  }
}

AssetPath AssetPath::split(String const& path) {
  auto i = path.begin();
  auto end = path.end();

  AssetPath components;

  // base paths cannot have any ':' or '?' characters, stop at the first one.
  while (i != end) {
    String::Char c = *i;
    if (c == ':' || c == '?')
      break;

    components.basePath += c;
    ++i;
  }

  // Sub-paths must immediately follow base paths and must start with a ':',
  // after this point any further ':' characters are not special.
  if (i != end && *i == ':') {
    ++i;
    while (i != end) {
      String::Char c = *i;
      if (c == '?')
        break;

      if (!components.subPath)
        components.subPath.emplace();

      *components.subPath += c;
      ++i;
    }
  }

  // Directives must follow the base path and optional sub-path, and each
  // directive is separated by one or more '?' characters.
  while (i != end && *i == '?') {
    String directives;
    while (i != end) {
      directives.append(*i);
      ++i;
    }

    if (!directives.empty())
      components.directives.append(move(directives));
  }

  starAssert(i == end);

  return components;
}

String AssetPath::join(AssetPath const& components) {
  return toString(components);
}

String AssetPath::setSubPath(String const& path, String const& subPath) {
  auto components = split(path);
  components.subPath = subPath;
  return join(components);
}

String AssetPath::removeSubPath(String const& path) {
  auto components = split(path);
  components.subPath.reset();
  return join(components);
}

String AssetPath::getDirectives(String const& path) {
  size_t firstDirective = path.find('?');
  if (firstDirective == NPos)
    return {};
  return path.substr(firstDirective + 1);
}

String AssetPath::addDirectives(String const& path, String const& directives) {
  return String::joinWith("?", path, directives);
}

String AssetPath::removeDirectives(String const& path) {
  size_t firstDirective = path.find('?');
  if (firstDirective == NPos)
    return path;
  return path.substr(0, firstDirective);
}

String AssetPath::directory(String const& path) {
  if (auto p = findFilenameRange(path.utf8())) {
    return String(path.utf8().substr(0, p->first));
  } else {
    return String();
  }
}

String AssetPath::filename(String const& path) {
  if (auto p = findFilenameRange(path.utf8())) {
    return String(path.utf8().substr(p->first, p->second));
  } else {
    return String();
  }
}

String AssetPath::extension(String const& path) {
  auto file = filename(path);
  auto lastDot = file.findLast(".");
  if (lastDot == NPos)
    return "";

  return file.substr(lastDot + 1);
}

String AssetPath::relativeTo(String const& sourcePath, String const& givenPath) {
  if (!givenPath.empty() && givenPath[0] == '/')
    return givenPath;

  auto path = directory(sourcePath);
  path.append(givenPath);
  return path;
}

bool AssetPath::operator==(AssetPath const& rhs) const {
  return tie(basePath, subPath, directives) == tie(rhs.basePath, rhs.subPath, rhs.directives);
}

AssetPath::AssetPath(const char* path) {
  *this = move(AssetPath::split(path));
}


AssetPath::AssetPath(String const& path) {
  *this = move(AssetPath::split(path));
}

AssetPath::AssetPath(String&& basePath, Maybe<String>&& subPath, DirectivesGroup&& directives) {
  this->basePath = move(basePath);
  this->subPath = move(subPath);
  this->directives = move(directives);
}

AssetPath::AssetPath(String const& basePath, Maybe<String> const& subPath, DirectivesGroup const& directives) {
  this->basePath = basePath;
  this->subPath = subPath;
  this->directives = directives;
}

std::ostream& operator<<(std::ostream& os, AssetPath const& rhs) {
  os << rhs.basePath;
  if (rhs.subPath) {
    os << ":";
    os << *rhs.subPath;
  }

  rhs.directives.forEach([&](auto const& entry) {
    os << "?";
    os << entry.string;
   });

  return os;
}

size_t hash<AssetPath>::operator()(AssetPath const& s) const {
  return hashOf(s.basePath, s.subPath, s.directives);
}

DataStream& operator>>(DataStream& ds, AssetPath& path) {
  String string;
  ds.read(string);

  path = move(string);

  return ds;
}

DataStream& operator<<(DataStream& ds, AssetPath const& path) {
  ds.write(AssetPath::join(path));

  return ds;
}

}
