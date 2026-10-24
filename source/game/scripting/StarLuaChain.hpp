#pragma once

#include "StarLua.hpp"
#include "StarDrawable.hpp"
#include "StarLightSource.hpp"
#include "StarEntityRenderingTypes.hpp"

namespace Star {

// Native version of the segment building loop in /items/active/effects/chain.lua.
namespace LuaChain {
  // Adds the drawables (and light sources) for a chain running from
  // startPosition along chainVec.  Follows chain.lua exactly (including
  // the 1/8 pixel scale that localAnimator.addDrawable applies).  Returns
  // false, having added nothing, if the chain uses something this does not
  // handle (jitter, waveform, non-numeric values, ...), in which case the
  // script should build it itself.
  bool addChain(Vec2D startPosition, Vec2D chainVec, LuaTable const& chain,
      List<pair<Drawable, Maybe<EntityRenderLayer>>>& drawables, List<LightSource>& lightSources);
}

}
