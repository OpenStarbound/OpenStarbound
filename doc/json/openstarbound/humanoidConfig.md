# humanoidConfig

---

## `Vec2F` headRotationCenter
The rotation center for the head, in pixels, default is `[0,-2]`

---

## `String` `JsonObject` animation
Is an optional string path or a JsonObject for an animation config, if it is defined the humanoid will use the networked animator for its animations instead of the entirely hardcoded humanoid animations.

The animation must contain these transformation groups as they are used for some hardcoded behavior that happens locally to the client.
- `headRotation`
- `backCosmetic<slot>Rotation` for slots 1-20
- `frontArmRotation`
- `backArmRotation`
- `personalityHeadOffset`
- `personalityArmOffset`

The animation must contain these stateTypes and states for some hardcoded behavior that happens locally to the client.
- `bodyDance`
    - `idle` set whenever the player/npc is not dancing or the dance step does not define a body frame
    - `dance` the default dance state set if the stateType does not contain a state matching the name of the body dance frame for the dance step
- `backArmDance`
    - `idle` set whenever the player/npc is not dancing or the dance step does not define a back arm frame
    - `dance` the default dance state set if the stateType does not contain a state matching the name of the back arm dance frame for the dance step
- `frontArmDance`
    - `idle` set whenever the player/npc is not dancing or the dance step does not define a front arm frame
    - `dance` the default dance state set if the stateType does not contain a state matching the name of the front arm dance frame for the dance step
- `backHandItem`
    - `inside` set when the held item is inside the hand
    - `outside` set when the held item is outside the hand
- `frontHandItem`
    - `inside` set when the held item is inside the hand
    - `outside` set when the held item is outside the hand
- `frontArm`
    - `idle` set when arm is not holding an item
    - `rotation` the default animation when holding an item and the stateType does not contain a state matching the name of the front arm rotation frame
- `backArm`
    - `idle` set when arm is not holding an item
    - `rotation` the default animation when holding an item and the stateType does not contain a state matching the name of the back arm rotation frame

All other animation data is entirely configurable.

For example, in the hardcoded humanoid animations, things like `walkBob` and `armWalkSeq` must be used and always behave in the same way, but in `/humanoid/opensb/build.lua` They are used to generate animation frame properties that imitate how retail used them, though this it not required, as the build script could do anything with its output.

## `JsonObject` stateAnimations
Each key must be a humanoid state `idle`, `walk`, `run`, `jump`, `fall`, `swim`, `swimIdle`, `duck`, `sit`, `lay` for a JsonObject that will control what animation states defined in the animation config are triggered when each state is triggered, this is so that when connecting to a retail server, these animations will trigger locally on your client.
here is an example
```
"stateAnimations" : {
    "idle" : {
        "body" : ["idle"]
    },
    "walk" : {
        "body" : ["walk"]
    }
}
```
`body` there being the name of a stateType in the animation, and `idle` being one of its states, one can also add additional args that are equivalent to calling `animator.setLocalAnimationState`

## `JsonObject` stateAnimationsBackwards
Same as `stateAnimations` but for when the player is moving backwards.
here is an example.
```
"stateAnimations" : {
    "idle" : {
        "body" : ["idle"]
    },
    "walk" : {
        "body" : ["walk", false, true]
    }
}
```
Note the additional arguments supplied to `setLocalAnimationState` in this example so that the walk animation will play backwards.

## `JsonObject` emoteAnimations
Same as `stateAnimations`, but instead triggers for humanoid emote states, `idle`, `blabber`, `shout`, `happy`, `sad`, `neutral`, `laugh`, `annoyed`, `oh`, `oooh`, `blink`, `wink`, `eat`, `sleep`

## `JsonObject` portraitAnimations
Same as `stateAnimations`, but instead triggers for portrait mode states, `head`, `bust`, `full`, `fullNeutral`, `fullNude`, `fullNeutralNude`
These are never triggered on the player/npc in world, but instead in the animator instanced for portraits.

---

## `JsonObject` identityFramesetTags
Key Value pairs of strings where the key is an animation tag and value is a string containing tags that will be replaced with active animation tags, these are set after identity has been set, so you could for example do
```
"identityFramesetTags" : {
    "bodyFrameset" : "<gender>body.png"
}
```

If any tag within the path is empty, the entire tag will be returned as empty. This is useful for parts such as facial hair/mask which often aren't always used by a species so the tag doesn't result in something like `facialHair/.png`.

Specific values are also used by humanoids which are not using the networked animator if they exist, and are used in place of the normal hardcoded paths. This can be used to for example, add a gender tag to arm parts, or remove the gender tag from body or head parts.
- `headFrameset`
- `bodyFrameset`
- `bodyMaskFrameset`
- `bodyHeadMaskFrameset`
- `emoteFrameset`
- `hairFrameset`
- `facialHairFrameset`
- `facialMaskFrameset`
- `backArmFrameset`
- `frontArmFrameset`
- `vaporTrailFrameset`

## `JsonObject` identityTags

Key value pairs for additional identity values that are used by identityFramesetTags and armor's humanoidAnimationTags that don't exist within retail starbound's identity struct.


---

## `String` frontHandItemPart
## `String` backHandItemPart
When using the animator, the part that item drawables will be attached to.
Defaults are `frontHandItem` and `backHandItem`

## `String` headRotationPart
## `String` headRotationPartPoint
When using the animator, the part point used as the rotation center.
Defaults are `head` and `rotationCenter`

## `String` frontArmRotationPart
## `String` frontArmRotationPartPoint
When using the animator, the part point used as the rotation center.
Defaults are `frontArm` and `rotationCenter`

## `String` backArmRotationPart
## `String` backArmRotationPartPoint
When using the animator, the part point used as the rotation center.
Defaults are `backArm` and `rotationCenter`

## `String` mouthOffsetPart
## `String` mouthOffsetPartPoint
When using the animator, the part point used as the mouth offset.
Defaults are `head` and `mouthOffset`

## `String` feetOffsetPart
## `String` feetOffsetPartPoint
When using the animator, the part point used as the feet offset.
Defaults are `body` and `feetOffset`

## `String` headArmorOffsetPart
## `String` headArmorOffsetPartPoint
When using the animator, the part point used as the head armor offset.
Defaults are `headCosmetic` and `armorOffset`

## `String` chestArmorOffsetPart
## `String` chestArmorOffsetPartPoint
When using the animator, the part point used as the head chest offset.
Defaults are `chestCosmetic` and `armorOffset`

## `String` legsArmorOffsetPart
## `String` legsArmorOffsetPartPoint
When using the animator, the part point used as the head legs offset.
Defaults are `legsCosmetic` and `armorOffset`

## `String` backArmorOffsetPart
## `String` backArmorOffsetPartPoint
When using the animator, the part point used as the head back offset.
Defaults are `backCosmetic` and `armorOffset`

---

## `List<String>` animationScripts
Client side animation scripts to be paired with the animation, does not need an animation to be defined to function.
Gets uninitialized and initialized with new scripts when the humanoid instance is refreshed.

---
