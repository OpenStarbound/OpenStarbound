#include "StarStatusControllerLuaBindings.hpp"
#include "StarStatusController.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeStatusControllerCallbacks(StatusController* statController) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<Json, String, Json>(
      "statusProperty", bind(StatusControllerCallbacks::statusProperty, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Json>(
      "setStatusProperty", bind(StatusControllerCallbacks::setStatusProperty, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String>(
      "stat", bind(StatusControllerCallbacks::stat, statController, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "statPositive", bind(StatusControllerCallbacks::statPositive, statController, _1));
  callbacks.registerCallbackWithSignature<StringList>(
      "resourceNames", bind(StatusControllerCallbacks::resourceNames, statController));
  callbacks.registerCallbackWithSignature<bool, String>(
      "isResource", bind(StatusControllerCallbacks::isResource, statController, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "resource", bind(StatusControllerCallbacks::resource, statController, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "resourcePositive", bind(StatusControllerCallbacks::resourcePositive, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "setResource", bind(StatusControllerCallbacks::setResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "modifyResource", bind(StatusControllerCallbacks::modifyResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "giveResource", bind(StatusControllerCallbacks::giveResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String, float>(
      "consumeResource", bind(StatusControllerCallbacks::consumeResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String, float>(
      "overConsumeResource", bind(StatusControllerCallbacks::overConsumeResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>(
      "resourceLocked", bind(StatusControllerCallbacks::resourceLocked, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setResourceLocked", bind(StatusControllerCallbacks::setResourceLocked, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "resetResource", bind(StatusControllerCallbacks::resetResource, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "resetAllResources", bind(StatusControllerCallbacks::resetAllResources, statController));
  callbacks.registerCallbackWithSignature<Maybe<float>, String>(
      "resourceMax", bind(StatusControllerCallbacks::resourceMax, statController, _1));
  callbacks.registerCallbackWithSignature<Maybe<float>, String>(
      "resourcePercentage", bind(StatusControllerCallbacks::resourcePercentage, statController, _1));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "setResourcePercentage", bind(StatusControllerCallbacks::setResourcePercentage, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "modifyResourcePercentage", bind(StatusControllerCallbacks::modifyResourcePercentage, statController, _1, _2));
  callbacks.registerCallbackWithSignature<JsonArray, String>(
      "getPersistentEffects", bind(StatusControllerCallbacks::getPersistentEffects, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, Json>(
      "addPersistentEffect", bind(StatusControllerCallbacks::addPersistentEffect, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, JsonArray>(
      "addPersistentEffects", bind(StatusControllerCallbacks::addPersistentEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, JsonArray>(
      "setPersistentEffects", bind(StatusControllerCallbacks::setPersistentEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "clearPersistentEffects", bind(StatusControllerCallbacks::clearPersistentEffects, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "clearAllPersistentEffects", bind(StatusControllerCallbacks::clearAllPersistentEffects, statController));
  callbacks.registerCallbackWithSignature<void, String, Maybe<float>, Maybe<EntityId>>(
      "addEphemeralEffect", bind(StatusControllerCallbacks::addEphemeralEffect, statController, _1, _2, _3));
  callbacks.registerCallbackWithSignature<void, JsonArray, Maybe<EntityId>>(
      "addEphemeralEffects", bind(StatusControllerCallbacks::addEphemeralEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "removeEphemeralEffect", bind(StatusControllerCallbacks::removeEphemeralEffect, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "clearEphemeralEffects", bind(StatusControllerCallbacks::clearEphemeralEffects, statController));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "damageTakenSince", bind(StatusControllerCallbacks::damageTakenSince, statController, _1));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "inflictedHitsSince", bind(StatusControllerCallbacks::inflictedHitsSince, statController, _1));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "inflictedDamageSince", bind(StatusControllerCallbacks::inflictedDamageSince, statController, _1));
  callbacks.registerCallbackWithSignature<List<JsonArray>>("activeUniqueStatusEffectSummary",
      bind(&StatusControllerCallbacks::activeUniqueStatusEffectSummary, statController));
  callbacks.registerCallbackWithSignature<bool, String>("uniqueStatusEffectActive",
      bind(&StatusControllerCallbacks::uniqueStatusEffectActive, statController, _1));

  callbacks.registerCallbackWithSignature<String>("primaryDirectives", bind(&StatusController::primaryDirectives, statController));
  callbacks.registerCallback("setPrimaryDirectives", [statController](Maybe<String> const& directives) { statController->setPrimaryDirectives(directives.value()); });

  callbacks.registerCallbackWithSignature<void, DamageRequest>("applySelfDamageRequest", bind(&StatusController::applySelfDamageRequest, statController, _1));

  return callbacks;
}

Json LuaBindings::StatusControllerCallbacks::statusProperty(
    StatusController* statController, String const& arg1, Json const& arg2) {
  return statController->statusProperty(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::setStatusProperty(
    StatusController* statController, String const& arg1, Json const& arg2) {
  statController->setStatusProperty(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::stat(StatusController* statController, String const& arg1) {
  return statController->stat(arg1);
}

bool LuaBindings::StatusControllerCallbacks::statPositive(StatusController* statController, String const& arg1) {
  return statController->statPositive(arg1);
}

StringList LuaBindings::StatusControllerCallbacks::resourceNames(StatusController* statController) {
  return statController->resourceNames();
}

bool LuaBindings::StatusControllerCallbacks::isResource(StatusController* statController, String const& arg1) {
  return statController->isResource(arg1);
}

float LuaBindings::StatusControllerCallbacks::resource(StatusController* statController, String const& arg1) {
  return statController->resource(arg1);
}

bool LuaBindings::StatusControllerCallbacks::resourcePositive(StatusController* statController, String const& arg1) {
  return statController->resourcePositive(arg1);
}

void LuaBindings::StatusControllerCallbacks::setResource(
    StatusController* statController, String const& arg1, float arg2) {
  statController->setResource(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::modifyResource(
    StatusController* statController, String const& arg1, float arg2) {
  statController->modifyResource(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::giveResource(
    StatusController* statController, String const& resourceName, float amount) {
  return statController->giveResource(resourceName, amount);
}

bool LuaBindings::StatusControllerCallbacks::consumeResource(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->consumeResource(arg1, arg2);
}

bool LuaBindings::StatusControllerCallbacks::overConsumeResource(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->overConsumeResource(arg1, arg2);
}

bool LuaBindings::StatusControllerCallbacks::resourceLocked(StatusController* statController, String const& arg1) {
  return statController->resourceLocked(arg1);
}

void LuaBindings::StatusControllerCallbacks::setResourceLocked(
    StatusController* statController, String const& arg1, bool arg2) {
  statController->setResourceLocked(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::resetResource(StatusController* statController, String const& arg1) {
  statController->resetResource(arg1);
}

void LuaBindings::StatusControllerCallbacks::resetAllResources(StatusController* statController) {
  statController->resetAllResources();
}

Maybe<float> LuaBindings::StatusControllerCallbacks::resourceMax(StatusController* statController, String const& arg1) {
  return statController->resourceMax(arg1);
}

Maybe<float> LuaBindings::StatusControllerCallbacks::resourcePercentage(
    StatusController* statController, String const& arg1) {
  return statController->resourcePercentage(arg1);
}

float LuaBindings::StatusControllerCallbacks::setResourcePercentage(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->setResourcePercentage(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::modifyResourcePercentage(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->modifyResourcePercentage(arg1, arg2);
}

JsonArray LuaBindings::StatusControllerCallbacks::getPersistentEffects(
    StatusController* statController, String const& arg1) {
  return statController->getPersistentEffects(arg1).transformed(jsonFromPersistentStatusEffect);
}

void LuaBindings::StatusControllerCallbacks::addPersistentEffect(
    StatusController* statController, String const& arg1, Json const& arg2) {
  addPersistentEffects(statController, arg1, JsonArray{arg2});
}

void LuaBindings::StatusControllerCallbacks::addPersistentEffects(
    StatusController* statController, String const& arg1, JsonArray const& arg2) {
  statController->addPersistentEffects(arg1, arg2.transformed(jsonToPersistentStatusEffect));
}

void LuaBindings::StatusControllerCallbacks::setPersistentEffects(
    StatusController* statController, String const& arg1, JsonArray const& arg2) {
  statController->setPersistentEffects(arg1, arg2.transformed(jsonToPersistentStatusEffect));
}

void LuaBindings::StatusControllerCallbacks::clearPersistentEffects(
    StatusController* statController, String const& arg1) {
  statController->clearPersistentEffects(arg1);
}

void LuaBindings::StatusControllerCallbacks::clearAllPersistentEffects(StatusController* statController) {
  statController->clearAllPersistentEffects();
}

void LuaBindings::StatusControllerCallbacks::addEphemeralEffect(
    StatusController* statController, String const& arg1, Maybe<float> arg2, Maybe<EntityId> sourceEntityId) {
  statController->addEphemeralEffect(EphemeralStatusEffect{UniqueStatusEffect{arg1}, arg2}, sourceEntityId);
}

void LuaBindings::StatusControllerCallbacks::addEphemeralEffects(
    StatusController* statController, JsonArray const& arg1, Maybe<EntityId> sourceEntityId) {
  statController->addEphemeralEffects(arg1.transformed(jsonToEphemeralStatusEffect), sourceEntityId);
}

void LuaBindings::StatusControllerCallbacks::removeEphemeralEffect(
    StatusController* statController, String const& arg1) {
  statController->removeEphemeralEffect(UniqueStatusEffect{arg1});
}

void LuaBindings::StatusControllerCallbacks::clearEphemeralEffects(StatusController* statController) {
  statController->clearEphemeralEffects();
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::damageTakenSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->damageTakenSince(timestep.value());
  return luaTupleReturn(pair.first.transformed(mem_fn(&DamageNotification::toJson)), pair.second);
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::inflictedHitsSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->inflictedHitsSince(timestep.value());
  return luaTupleReturn(
      pair.first.transformed([](auto const& p) { return p.second.toJson().set("targetEntityId", p.first); }),
      pair.second);
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::inflictedDamageSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->inflictedDamageSince(timestep.value());
  return luaTupleReturn(pair.first.transformed(mem_fn(&DamageNotification::toJson)), pair.second);
}

List<JsonArray> LuaBindings::StatusControllerCallbacks::activeUniqueStatusEffectSummary(
    StatusController* statController) {
  auto summary = statController->activeUniqueStatusEffectSummary();
  return summary.transformed([](pair<UniqueStatusEffect, Maybe<float>> effect) {
    JsonArray effectJson = {effect.first};
    if (effect.second)
      effectJson.append(*effect.second);
    return effectJson;
  });
}

bool LuaBindings::StatusControllerCallbacks::uniqueStatusEffectActive(
    StatusController* statController, String const& effectName) {
  return statController->uniqueStatusEffectActive(effectName);
}

}
