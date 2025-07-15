#include "StarJsonPatch.hpp"
#include "StarJsonPath.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

Json jsonPatch(Json const& base, JsonArray const& patch) {
  auto res = base;
  try {
    for (auto const& operation : patch) {
      res = JsonPatching::applyOperation(res, operation);
    }
    return res;
  } catch (JsonException const& e) {
    throw JsonPatchException(strf("Could not apply patch to base. {}", e.what()), false);
  }
}


// Returns 0 if not found, index + 1 if found.
size_t findJsonMatch(Json const& searchable, Json const& value, JsonPath::Pointer& pointer) {
  if (searchable.isType(Json::Type::Array)) {
    auto array = searchable.toArray();
    for (size_t i = 0; i != array.size(); ++i) {
      if (jsonPartialMatch(array[i], value))
        return i + 1;
    }
  } else {
    throw JsonPatchException(strf("Search operation failure, value at '{}' is not an array.", pointer.path()), false);
  }
  return 0;
}


namespace JsonPatching {

  static const StringMap<std::function<Json(Json, Json)>> functionMap = StringMap<std::function<Json(Json, Json)>>{
      {"test", std::bind(applyTestOperation, _1, _2)},
      {"remove", std::bind(applyRemoveOperation, _1, _2)},
      {"add", std::bind(applyAddOperation, _1, _2)},
      {"replace", std::bind(applyReplaceOperation, _1, _2)},
      {"move", std::bind(applyMoveOperation, _1, _2)},
      {"copy", std::bind(applyCopyOperation, _1, _2)},
      {"merge", std::bind(applyMergeOperation, _1, _2)},
  };

  Json applyOperation(Json const& base, Json const& op, Maybe<Json> const&) {
    try {
      auto operation = op.getString("op");
      return JsonPatching::functionMap.get(operation)(base, op);
    } catch (JsonException const& e) {
      throw JsonPatchException(strf("Could not apply operation to base. {}", e.what()), false);
    } catch (MapException const&) {
      throw JsonPatchException(strf("Invalid operation: {}", op.getString("op")), false);
    }
  }

  Json applyTestOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);
    auto inverseTest = op.getBool("inverse", false);

    try {
      if (op.contains("search")) {
        auto searchable = pointer.get(base);
        auto searchValue = op.get("search");
        bool found = findJsonMatch(searchable, searchValue, pointer);
        if (found && inverseTest)
          throw JsonPatchTestFail(strf("Test operation failure, expected {} to be missing.", searchValue), false);
        else if (!found && !inverseTest)
          throw JsonPatchTestFail(strf("Test operation failure, could not find {}.", searchValue), false);
        return base;
      } else {
        auto value = op.opt("value");
        auto testValue = pointer.get(base);
        if (!value) {
          if (inverseTest)
            throw JsonPatchTestFail(strf("Test operation failure, expected {} to be missing.", path), false);
          return base;
        }

        if ((value && (testValue == *value)) ^ inverseTest)
          return base;
        else
          throw JsonPatchTestFail(strf("Test operation failure, expected {} found {}.", value, testValue), false);
      }
    } catch (JsonPath::TraversalException& e) {
      if (inverseTest)
        return base;
      throw JsonPatchTestFail(strf("Test operation failure: {}", e.what()), false);
    }
  }

  Json applyRemoveOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto searchable = pointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer))
        return pointer.add(pointer.remove(base), searchable.eraseIndex(index - 1));
      else
        return base;
    } else {
      return pointer.remove(base);
    }
  }

  Json applyAddOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto value = op.get("value");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto searchable = pointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer))
        return pointer.add(pointer.remove(base), searchable.insert(index - 1, value));
      else
        return base;
    } else {
      return pointer.add(base, value);
    }
  }

  Json applyReplaceOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto value = op.get("value");
    auto pointer = JsonPath::Pointer(op.getString("path"));

    if (op.contains("search")) {
      auto searchable = pointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer))
        return pointer.add(pointer.remove(base), searchable.set(index - 1, value));
      else
        return base;
    } else {
      return pointer.add(pointer.remove(base), value);
    }
  }

  Json applyMoveOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto toPointer = JsonPath::Pointer(path);
    auto fromPointer = JsonPath::Pointer(op.getString("from"));

    if (op.contains("search")) {
      auto searchable = fromPointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, fromPointer)) {
        auto result = toPointer.add(base, searchable.get(index - 1));
        return fromPointer.add(result, searchable.eraseIndex(index - 1));
      }
      else
        return base;
    } else {
      Json value = fromPointer.get(base);
      return toPointer.add(fromPointer.remove(base), value);
    }
  }

  Json applyCopyOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto toPointer = JsonPath::Pointer(path);
    auto fromPointer = JsonPath::Pointer(op.getString("from"));

    if (op.contains("search")) {
      auto searchable = fromPointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, fromPointer))
        return toPointer.add(base, searchable.get(index - 1));
      else
        return base;
    } else {
      Json value = fromPointer.get(base);
      return toPointer.add(base, value);
    }
  }

  Json applyMergeOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto searchable = pointer.get(base);
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer))
        return pointer.add(pointer.remove(base), searchable.set(index - 1, jsonMerge(searchable.get(index - 1), op.get("value"))));
      else
        return base;
    } else {
      return pointer.add(pointer.remove(base), jsonMerge(pointer.get(base), op.get("value")));
    }
  }
}

}
