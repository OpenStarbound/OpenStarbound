## `bool` statusEffectsInCosmeticSlots
If true enables the armor to apply its status effects even when in cosmetic slots

## `List<PersistentStatusEffect>` cosmeticStatusEffects
List of status effects that apply whenever this armor is visible

## `JsonObject` humanoidAnimationTags
When a humanoid is using an animation config, the key value pairs of this table will be set as global tags in the animation.

The `<slot>` tag within any keys will be replaced with the slot index of the armor slot (1-20)

The `<directory>` tag within any value will be replaced with the armor's directory.

Any additional tags will be replaced with the current tags at the time of equipping the armor. So for example things like `<species>` and `<gender>` will be replaced with the relevant species and gender, as the humanoid animator always sets those tags.

## `bool` bypassNude
If true enables the armor to appear even when its wearer is in a nude state. Useful for body parts (tails, hair, etc)

## `bool` rotateWithHead
Applies to backArmor. If true enables the armor to rotate along with the wearer's head.
