#pragma once

#include "StarJson.hpp"

namespace Star {

STAR_EXCEPTION(JsonPatchException, JsonException);
STAR_EXCEPTION(JsonPatchTestFail, StarException);

// Applies the given RFC6902 compliant patch to the base and returns the result
// Throws JsonPatchException on patch failure.
Json jsonPatch(Json const& base, JsonArray const& patch);

namespace JsonPatching {
  // Applies the given single operation
  Json applyOperation(Json const& base, Json const& op);

  // Tests for "value" at "path"
  // Returns base or throws JsonPatchException
  Json applyTestOperation(Json const& base, Json const& op);

  // Removes the value at "path"
  Json applyRemoveOperation(Json const& base, Json const& op);

  // Adds "value" at "path"
  Json applyAddOperation(Json const& base, Json const& op);

  // Replaces "path" with "value"
  Json applyReplaceOperation(Json const& base, Json const& op);

  // Moves "from" to "path"
  Json applyMoveOperation(Json const& base, Json const& op);

  // Copies "from" to "path"
  Json applyCopyOperation(Json const& base, Json const& op);
}

}
