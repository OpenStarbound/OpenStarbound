#ifndef STAR_JSON_PATH_HPP
#define STAR_JSON_PATH_HPP

#include "StarLexicalCast.hpp"
#include "StarJson.hpp"

namespace Star {

namespace JsonPath {
  enum class TypeHint {
    Array,
    Object
  };

  typedef function<TypeHint(String&, String const&, String::const_iterator&, String::const_iterator)> PathParser;

  STAR_EXCEPTION(ParsingException, JsonException);
  STAR_EXCEPTION(TraversalException, JsonException);

  // Parses RFC 6901 JSON Pointers, e.g. /foo/bar/4/baz
  TypeHint parsePointer(String& outputBuffer, String const& path, String::const_iterator& iterator, String::const_iterator end);

  // Parses JavaScript-like paths, e.g. foo.bar[4].baz
  TypeHint parseQueryPath(String& outputBuffer, String const& path, String::const_iterator& iterator, String::const_iterator end);

  // Retrieves the portion of the Json document referred to by the given path.
  template <typename Jsonlike>
  Jsonlike pathGet(Jsonlike base, PathParser parser, String const& path);

  // Find a given portion of the JSON document, if it exists.  Instead of
  // throwing a TraversalException if a portion of the path is invalid, simply
  // returns nothing.
  template <typename Jsonlike>
  Maybe<Jsonlike> pathFind(Jsonlike base, PathParser parser, String const& path);

  template <typename Jsonlike>
  using JsonOp = function<Jsonlike(Jsonlike const&, Maybe<String> const&)>;

  // Applies a function to the portion of the Json document referred to by the
  // given path, returning the resulting new document.  If the end of the path
  // doesn't exist, the JsonOp is called with None, and its result will be
  // inserted into the document.  If the path already existed and the JsonOp
  // returns None, it is erased.  This is not as well-optimized as pathGet, but
  // also not on the critical path for anything.
  template <typename Jsonlike>
  Jsonlike pathApply(Jsonlike const& base, PathParser parser, String const& path, JsonOp<Jsonlike> op);

  // Sets a value on a Json document at the location referred to by path,
  // returning the resulting new document.
  template <typename Jsonlike>
  Jsonlike pathSet(Jsonlike const& base, PathParser parser, String const& path, Jsonlike const& value);

  // Erases the location referred to by the path from the document
  template <typename Jsonlike>
  Jsonlike pathRemove(Jsonlike const& base, PathParser parser, String const& path);

  // Performs RFC6902 (JSON Patching) add operation. Inserts into arrays, or
  // appends if the last path segment is "-". On objects, does the same as
  // pathSet.
  template <typename Jsonlike>
  Jsonlike pathAdd(Jsonlike const& base, PathParser parser, String const& path, Jsonlike const& value);

  template <typename Jsonlike>
  using EmptyPathOp = function<Jsonlike(Jsonlike const&)>;
  template <typename Jsonlike>
  using ObjectOp = function<Jsonlike(Jsonlike const&, String const&)>;
  template <typename Jsonlike>
  using ArrayOp = function<Jsonlike(Jsonlike const&, Maybe<size_t>)>;

  template <typename Jsonlike>
  JsonOp<Jsonlike> genericObjectArrayOp(String path, EmptyPathOp<Jsonlike> emptyPathOp, ObjectOp<Jsonlike> objectOp, ArrayOp<Jsonlike> arrayOp);

  STAR_CLASS(Path);
  STAR_CLASS(Pointer);
  STAR_CLASS(QueryPath);

  class Path {
  public:
    Path(PathParser parser, String const& path) : m_parser(parser), m_path(path) {}

    template <typename Jsonlike>
    Jsonlike get(Jsonlike const& base) {
      return pathGet(base, m_parser, m_path);
    }

    template <typename Jsonlike>
    Jsonlike apply(Jsonlike const& base, JsonOp<Jsonlike> op) {
      return pathApply(base, m_parser, m_path, op);
    }

    template <typename Jsonlike>
    Jsonlike apply(Jsonlike const& base,
        EmptyPathOp<Jsonlike> emptyPathOp,
        ObjectOp<Jsonlike> objectOp,
        ArrayOp<Jsonlike> arrayOp) {
      JsonOp<Jsonlike> combinedOp = genericObjectArrayOp(m_path, emptyPathOp, objectOp, arrayOp);
      return pathApply(base, m_parser, m_path, combinedOp);
    }

    template <typename Jsonlike>
    Jsonlike set(Jsonlike const& base, Jsonlike const& value) {
      return pathSet(base, m_parser, m_path, value);
    }

    template <typename Jsonlike>
    Jsonlike remove(Jsonlike const& base) {
      return pathRemove(base, m_parser, m_path);
    }

    template <typename Jsonlike>
    Jsonlike add(Jsonlike const& base, Jsonlike const& value) {
      return pathAdd(base, m_parser, m_path, value);
    }

    String const& path() const {
      return m_path;
    }

  private:
    PathParser m_parser;
    String m_path;
  };

  class Pointer : public Path {
  public:
    Pointer(String const& path) : Path(parsePointer, path) {}
  };

  class QueryPath : public Path {
  public:
    QueryPath(String const& path) : Path(parseQueryPath, path) {}
  };

  template <typename Jsonlike>
  Jsonlike pathGet(Jsonlike value, PathParser parser, String const& path) {
    String buffer;
    buffer.reserve(path.size());

    auto pos = path.begin();

    while (pos != path.end()) {
      parser(buffer, path, pos, path.end());

      if (value.type() == Json::Type::Array) {
        if (buffer == "-")
          throw TraversalException::format("Tried to get key '%s' in non-object type in pathGet(\"%s\")", buffer, path);
        Maybe<size_t> i = maybeLexicalCast<size_t>(buffer);
        if (!i)
          throw TraversalException::format("Cannot parse '%s' as index in pathGet(\"%s\")", buffer, path);

        if (*i < value.size())
          value = value.get(*i);
        else
          throw TraversalException::format("Index %s out of range in pathGet(\"%s\")", buffer, path);

      } else if (value.type() == Json::Type::Object) {
        if (value.contains(buffer))
          value = value.get(buffer);
        else
          throw TraversalException::format("No such key '%s' in pathGet(\"%s\")", buffer, path);

      } else {
        throw TraversalException::format("Tried to get key '%s' in non-object type in pathGet(\"%s\")", buffer, path);
      }
    }
    return value;
  }

  template <typename Jsonlike>
  Maybe<Jsonlike> pathFind(Jsonlike value, PathParser parser, String const& path) {
    String buffer;
    buffer.reserve(path.size());

    auto pos = path.begin();

    while (pos != path.end()) {
      parser(buffer, path, pos, path.end());

      if (value.type() == Json::Type::Array) {
        if (buffer == "-")
          return {};

        Maybe<size_t> i = maybeLexicalCast<size_t>(buffer);
        if (i && *i < value.size())
          value = value.get(*i);
        else
          return {};

      } else if (value.type() == Json::Type::Object) {
        if (value.contains(buffer))
          value = value.get(buffer);
        else
          return {};

      } else {
        return {};
      }
    }
    return value;
  }

  template <typename Jsonlike>
  Jsonlike pathApply(String& buffer,
      Jsonlike const& value,
      PathParser parser,
      String const& path,
      String::const_iterator const current,
      JsonOp<Jsonlike> op) {
    if (current == path.end())
      return op(value, {});

    String::const_iterator iterator = current;
    parser(buffer, path, iterator, path.end());

    if (value.type() == Json::Type::Array) {
      if (iterator == path.end()) {
        return op(value, buffer);
      } else {
        Maybe<size_t> i = maybeLexicalCast<size_t>(buffer);
        if (!i)
          throw TraversalException::format("Cannot parse '%s' as index in pathApply(\"%s\")", buffer, path);

        if (*i >= value.size())
          throw TraversalException::format("Index %s out of range in pathApply(\"%s\")", buffer, path);

        return value.set(*i, pathApply(buffer, value.get(*i), parser, path, iterator, op));
      }

    } else if (value.type() == Json::Type::Object) {
      if (iterator == path.end()) {
        return op(value, buffer);

      } else {
        if (!value.contains(buffer))
          throw TraversalException::format("No such key '%s' in pathApply(\"%s\")", buffer, path);

        Jsonlike newChild = pathApply(buffer, value.get(buffer), parser, path, iterator, op);
        iterator = current;
        // pathApply just mutated buffer. Recover the current path component:
        parser(buffer, path, iterator, path.end());
        return value.set(buffer, newChild);
      }

    } else {
      throw TraversalException::format("Tried to get key '%s' in non-object type in pathApply(\"%s\")", buffer, path);
    }
  }

  template <typename Jsonlike>
  Jsonlike pathApply(Jsonlike const& base, PathParser parser, String const& path, JsonOp<Jsonlike> op) {
    String buffer;
    return pathApply(buffer, base, parser, path, path.begin(), op);
  }

  template <typename Jsonlike>
  JsonOp<Jsonlike> genericObjectArrayOp(String path, EmptyPathOp<Jsonlike> emptyPathOp, ObjectOp<Jsonlike> objectOp, ArrayOp<Jsonlike> arrayOp) {
    return [=](Jsonlike const& parent, Maybe<String> const& key) -> Jsonlike {
      if (key.isNothing())
        return emptyPathOp(parent);
      if (parent.type() == Json::Type::Array) {
        if (*key == "-")
          return arrayOp(parent, {});
        Maybe<size_t> i = maybeLexicalCast<size_t>(*key);
        if (!i)
          throw TraversalException::format("Cannot parse '%s' as index in Json path \"%s\"", *key, path);
        if (i && *i > parent.size())
          throw TraversalException::format("Index %s out of range in Json path \"%s\"", *key, path);
        if (i && *i == parent.size())
          i = {};
        return arrayOp(parent, i);
      } else if (parent.type() == Json::Type::Object) {
        return objectOp(parent, *key);
      } else {
        throw TraversalException::format("Tried to set key '%s' in non-object type in pathSet(\"%s\")", *key, path);
      }
    };
  }

  template <typename Jsonlike>
  Jsonlike pathSet(Jsonlike const& base, PathParser parser, String const& path, Jsonlike const& value) {
    EmptyPathOp<Jsonlike> emptyPathOp = [&value](Jsonlike const&) {
      return value;
    };
    ObjectOp<Jsonlike> objectOp = [&value](Jsonlike const& object, String const& key) {
      return object.set(key, value);
    };
    ArrayOp<Jsonlike> arrayOp = [&value](Jsonlike const& array, Maybe<size_t> i) {
      if (i.isValid())
        return array.set(*i, value);
      return array.append(value);
    };
    return pathApply(base, parser, path, genericObjectArrayOp(path, emptyPathOp, objectOp, arrayOp));
  }

  template <typename Jsonlike>
  Jsonlike pathRemove(Jsonlike const& base, PathParser parser, String const& path) {
    EmptyPathOp<Jsonlike> emptyPathOp = [](Jsonlike const&) { return Json{}; };
    ObjectOp<Jsonlike> objectOp = [](Jsonlike const& object, String const& key) {
      if (!object.contains(key))
        throw TraversalException::format("Could not find \"%s\" to remove", key);
      return object.eraseKey(key);
    };
    ArrayOp<Jsonlike> arrayOp = [](Jsonlike const& array, Maybe<size_t> i) {
      if (i.isValid())
        return array.eraseIndex(*i);
      throw TraversalException("Could not remove element after end of array");
    };
    return pathApply(base, parser, path, genericObjectArrayOp(path, emptyPathOp, objectOp, arrayOp));
  }

  template <typename Jsonlike>
  Jsonlike pathAdd(Jsonlike const& base, PathParser parser, String const& path, Jsonlike const& value) {
    EmptyPathOp<Jsonlike> emptyPathOp = [&value](Jsonlike const& document) {
      if (document.type() == Json::Type::Null)
        return value;
      throw JsonException("Cannot add a value to the entire document, it is not empty.");
    };
    ObjectOp<Jsonlike> objectOp = [&value](Jsonlike const& object, String const& key) {
      return object.set(key, value);
    };
    ArrayOp<Jsonlike> arrayOp = [&value](Jsonlike const& array, Maybe<size_t> i) {
      if (i.isValid())
        return array.insert(*i, value);
      return array.append(value);
    };
    return pathApply(base, parser, path, genericObjectArrayOp(path, emptyPathOp, objectOp, arrayOp));
  }
}

}

#endif
