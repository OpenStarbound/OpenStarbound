## `bool` statusEffectsInCosmeticSlots
If true enables the armor to apply its status effects even when in cosmetic slots

## `List<PersistentStatusEffect>` cosmeticStatusEffects
List of status effects that apply whenever this armor is visible

## `JsonObject` humanoidAnimationTags
When a humanoid is using an animation config, the key value pairs of this table will be set as global tags in the animation.

The `<slot>` tag within any keys will be replaced with the slot index of the armor slot (1-20)

The `<directory>` tag within any value will be replaced with the armor's directory.

Any additional tags will be replaced with the current identity tags. So for example things like `<species>` and `<gender>` will be replaced with the relevant species and gender of the entity equipping it.
