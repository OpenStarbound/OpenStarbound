#ifndef STAR_LUA_GAME_CONVERTERS_HPP
#define STAR_LUA_GAME_CONVERTERS_HPP

#include "StarLuaConverters.hpp"
#include "StarCollisionBlock.hpp"
#include "StarPlatformerAStar.hpp"
#include "StarActorMovementController.hpp"
#include "StarDamage.hpp"
#include "StarCollectionDatabase.hpp"
#include "StarBehaviorState.hpp"
#include "StarSystemWorld.hpp"
#include "StarDrawable.hpp"

namespace Star {

template <>
struct LuaConverter<CollisionKind> {
  static LuaValue from(LuaEngine& engine, CollisionKind k);
  static Maybe<CollisionKind> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<CollisionSet> {
  static LuaValue from(LuaEngine& engine, CollisionSet const& s);
  static Maybe<CollisionSet> to(LuaEngine& engine, LuaValue const& v);
};

template <typename T>
struct LuaConverter<RpcPromise<T>> : LuaUserDataConverter<RpcPromise<T>> {};

template <typename T>
struct LuaUserDataMethods<RpcPromise<T>> {
  static LuaMethods<RpcPromise<T>> make();
};

template <>
struct LuaConverter<PlatformerAStar::Path> {
  static LuaValue from(LuaEngine& engine, PlatformerAStar::Path const& path);
};

template <>
struct LuaConverter<PlatformerAStar::PathFinder> : LuaUserDataConverter<PlatformerAStar::PathFinder> {};

template <>
struct LuaUserDataMethods<PlatformerAStar::PathFinder> {
  static LuaMethods<PlatformerAStar::PathFinder> make();
};

template <>
struct LuaConverter<PlatformerAStar::Parameters> {
  static Maybe<PlatformerAStar::Parameters> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<ActorJumpProfile> {
  static LuaValue from(LuaEngine& engine, ActorJumpProfile const& v);
  static Maybe<ActorJumpProfile> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<ActorMovementParameters> {
  static LuaValue from(LuaEngine& engine, ActorMovementParameters const& v);
  static Maybe<ActorMovementParameters> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<ActorMovementModifiers> {
  static LuaValue from(LuaEngine& engine, ActorMovementModifiers const& v);
  static Maybe<ActorMovementModifiers> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<StatModifier> {
  static LuaValue from(LuaEngine& engine, StatModifier const& v);
  static Maybe<StatModifier> to(LuaEngine& engine, LuaValue v);
};

template <>
struct LuaConverter<EphemeralStatusEffect> {
  static LuaValue from(LuaEngine& engine, EphemeralStatusEffect const& v);
  static Maybe<EphemeralStatusEffect> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<DamageRequest> {
  static LuaValue from(LuaEngine& engine, DamageRequest const& v);
  static Maybe<DamageRequest> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<DamageNotification> {
  static LuaValue from(LuaEngine& engine, DamageNotification const& v);
  static Maybe<DamageNotification> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<LiquidLevel> {
  static LuaValue from(LuaEngine& engine, LiquidLevel const& v);
  static Maybe<LiquidLevel> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<Drawable> {
  static LuaValue from(LuaEngine& engine, Drawable const& v);
  static Maybe<Drawable> to(LuaEngine& engine, LuaValue const& v);
};

template <typename T>
LuaMethods<RpcPromise<T>> LuaUserDataMethods<RpcPromise<T>>::make() {
  LuaMethods<RpcPromise<T>> methods;
  methods.template registerMethodWithSignature<bool, RpcPromise<T>&>("finished", mem_fn(&RpcPromise<T>::finished));
  methods.template registerMethodWithSignature<bool, RpcPromise<T>&>("succeeded", mem_fn(&RpcPromise<T>::succeeded));
  methods.template registerMethodWithSignature<Maybe<T>, RpcPromise<T>&>("result", mem_fn(&RpcPromise<T>::result));
  methods.template registerMethodWithSignature<Maybe<String>, RpcPromise<T>&>("error", mem_fn(&RpcPromise<T>::error));
  return methods;
}

template <>
struct LuaConverter<Collection> {
  static LuaValue from(LuaEngine& engine, Collection const& c);
  static Maybe<Collection> to(LuaEngine& engine, LuaValue const& v);
};

template <>
struct LuaConverter<Collectable> {
  static LuaValue from(LuaEngine& engine, Collectable const& c);
  static Maybe<Collectable> to(LuaEngine& engine, LuaValue const& v);
};

// BehaviorState contains Lua references, putting it in a UserData violates
// the "don't put lua references in userdata, just don't" rule. We get around it by keeping
// a weak pointer to the behavior state, forcing it to be destroyed elsewhere.
template <>
struct LuaConverter<BehaviorStateWeakPtr> : LuaUserDataConverter<BehaviorStateWeakPtr> {};

template <>
struct LuaUserDataMethods<BehaviorStateWeakPtr> {
  static LuaMethods<BehaviorStateWeakPtr> make();
};

template <>
struct LuaConverter<NodeStatus> {
  static LuaValue from(LuaEngine& engine, NodeStatus const& status);
  static NodeStatus to(LuaEngine& engine, LuaValue const& v);
};

// Weak pointer for the same reasons as BehaviorState.
template <>
struct LuaConverter<BlackboardWeakPtr> : LuaUserDataConverter<BlackboardWeakPtr> {};

template <>
struct LuaUserDataMethods<BlackboardWeakPtr> {
  static LuaMethods<BlackboardWeakPtr> make();
};

}

#endif
