#pragma once

#include "StarLua.hpp"
#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(StatusController);

namespace LuaBindings {
  LuaCallbacks makeStatusControllerCallbacks(StatusController* statController);

  namespace StatusControllerCallbacks {
    Json statusProperty(StatusController* statController, String const& arg1, Json const& arg2);
    void setStatusProperty(StatusController* statController, String const& arg1, Json const& arg2);
    float stat(StatusController* statController, String const& arg1);
    bool statPositive(StatusController* statController, String const& arg1);
    StringList resourceNames(StatusController* statController);
    bool isResource(StatusController* statController, String const& arg1);
    float resource(StatusController* statController, String const& arg1);
    bool resourcePositive(StatusController* statController, String const& arg1);
    void setResource(StatusController* statController, String const& arg1, float arg2);
    void modifyResource(StatusController* statController, String const& arg1, float arg2);
    float giveResource(StatusController* statController, String const& resourceName, float amount);
    bool consumeResource(StatusController* statController, String const& arg1, float arg2);
    bool overConsumeResource(StatusController* statController, String const& arg1, float arg2);
    bool resourceLocked(StatusController* statController, String const& arg1);
    void setResourceLocked(StatusController* statController, String const& arg1, bool arg2);
    void resetResource(StatusController* statController, String const& arg1);
    void resetAllResources(StatusController* statController);
    Maybe<float> resourceMax(StatusController* statController, String const& arg1);
    Maybe<float> resourcePercentage(StatusController* statController, String const& arg1);
    float setResourcePercentage(StatusController* statController, String const& arg1, float arg2);
    float modifyResourcePercentage(StatusController* statController, String const& arg1, float arg2);
    JsonArray getPersistentEffects(StatusController* statController, String const& arg1);
    void addPersistentEffect(StatusController* statController, String const& arg1, Json const& arg2, Maybe<EntityId> sourceEntityId = {});
    void addPersistentEffects(StatusController* statController, String const& arg1, JsonArray const& arg2, Maybe<EntityId> sourceEntityId = {});
    void setPersistentEffects(StatusController* statController, String const& arg1, JsonArray const& arg2, Maybe<EntityId> sourceEntityId = {});
    void clearPersistentEffects(StatusController* statController, String const& arg1);
    void clearAllPersistentEffects(StatusController* statController);
    void addEphemeralEffect(StatusController* statController,
        String const& uniqueEffect,
        Maybe<float> duration,
        Maybe<EntityId> sourceEntityId);
    void addEphemeralEffects(StatusController* statController, JsonArray const& arg1, Maybe<EntityId> sourceEntityId);
    void removeEphemeralEffect(StatusController* statController, String const& arg1);
    void clearEphemeralEffects(StatusController* statController);
    LuaTupleReturn<List<Json>, uint64_t> damageTakenSince(StatusController* statController, Maybe<uint64_t> timestep);
    LuaTupleReturn<List<Json>, uint64_t> inflictedHitsSince(StatusController* statController, Maybe<uint64_t> timestep);
    LuaTupleReturn<List<Json>, uint64_t> inflictedDamageSince(
        StatusController* statController, Maybe<uint64_t> timestep);
    List<JsonArray> activeUniqueStatusEffectSummary(StatusController* statController);
    bool uniqueStatusEffectActive(StatusController* statController, String const& effectName);
  }
}
}
