# npc

The `npc` table is for functions relating directly to the current npc. It is available only in NPC scripts.

---

#### `void` npc.setHumanoidIdentity(`Json` humanoidIdentity)

Sets the specific humanoid identity of the npc.

Implicitly calls `npc.refreshHumanoidParameters()` if species or imagePath is changed.

---

#### `String` npc.name()

Returns the npc's name.

#### `void` npc.setName(`String` name)

Sets the npc's name.

---

#### `String` npc.description()

Returns the npc's description.

#### `void` npc.setDescription(`String` description)

Sets the npc's description. The new description will not be networked until the npc is re-loaded.

---

#### `void` npc.refreshHumanoidParameters()

Refreshes the active humanoid parameters by re-building the current humanoid instance.

This must be called when certain values in the humanoid config would be changed by a parameter, such as the animationConfig and movementParameters.

This is implicitly called whenever the current humanoid identity's species or imagePath changes.

This has to network the humanoid identity and humanoid parameters as intialization arguments for the new instance, so only use it when necessary.

#### `void` npc.setHumanoidParameter(`String` key `Json` value)

Sets a parameter that overwrites a value in the humanoid config.

#### `void` npc.setHumanoidParameters(`JsonObject` parameters)

Replaces the current table of humanoid parameters that overwrite values in the humanoid config.

#### `Maybe<Json>` npc.getHumanoidParameter(`String` key)

Gets a humanoid parameter.

#### `JsonObject` npc.getHumanoidParameters()

Gets a table of all the humanoid parameters.

---

#### `JsonObject` npc.humanoidConfig(`bool` withOverrides)

Gets the active humanoid config.

If withOverrides is true, returns it with the overrides applied by armor.

The humanoid config controls how a the npc is animated, as well as their movement properties.

---

#### `void` npc.setSpecies(`String` species)

Sets the npc's species. Must be a valid species.

Implicitly calls `npc.refreshHumanoidParameters()` if species is changed.

---

#### `void` npc.setGender(`String` gender)

Sets the npc's gender.

---

#### `String` npc.imagePath()

If the npc has a custom humanoid image path set, returns it. otherwise, returns `nil`.

#### `void` npc.setImagePath(`String` imagePath)

Sets the npc's image path. Specify `nil` to remove the image path.

Implicitly calls `npc.refreshHumanoidParameters()` if imagePath is changed.

---

#### `Personality` npc.personality()

Returns the npc's personality as a `table` containing a `string` idle, `string` armIdle, `Vec2F` headOffset and `Vec2F` armOffset.

#### `void` npc.setPersonality(`Personality` personality)

Sets the npc's personality. The **personality** must be a `table` containing at least one value as returned by `npc.personality()`.

---

#### `String` npc.bodyDirectives()

Returns the npc's body directives.

#### `void` npc.setBodyDirectives(`String` bodyDirectives)

Sets the npc's body directives.

---

#### `String` npc.emoteDirectives()

Returns the npc's emote directives.

#### `void` npc.setEmoteDirectives(`String` emoteDirectives)

Sets the npc's emote directives.

---

#### `void` npc.setHair(`String` hairGroup, `String` hairType, `String` hairDirectives)

Sets the npc's hair group, type, and directives.

---

#### `String` npc.hairGroup()

Returns the npc's hair group.

#### `void` npc.setHairGroup(`String` hairGroup)

Sets the npc's hair group.

---

#### `String` npc.hairType()

Returns the npc's hair type.

#### `void` npc.setHairType(`String` hairType)

Sets the npc's hair type.

---

#### `String` npc.hairDirectives()

Returns the npc's hair directives.

#### `void` npc.setHairDirectives(`String` hairDirectives)

Sets the npc's hair directives.

---

#### `String` npc.facialHair()

Returns the npc's facial hair type. Same as npc.facialHairType?

#### `void` npc.setFacialHair(`String` facialHairGroup, `String` facialHairType, `String` facialHairDirectives)

Sets the npc's facial hair group, type, and directives.

---

#### `String` npc.facialHairType()

Returns the npc's facial hair type.

#### `void` npc.setFacialHairType(`String` facialHairType)

Sets the npc's facial hair type.

---

#### `String` npc.facialHairGroup()

Returns the npc's facial hair group.

#### `void` npc.setFacialHairGroup(`String` facialHairGroup)

Sets the npc's facial hair group.

---

#### `String` npc.facialHairDirectives()

Returns the npc's facial hair directives.

#### `void` npc.setFacialHairDirectives(`String` facialHairDirectives)

Sets the npc's facial hair directives.

---

#### `String` npc.facialMask()

Returns the npc's facial mask group.

#### `void` npc.setFacialMask(`String` facialMaskGroup, `String` facialMaskType, `String` facialMaskDirectives)

Sets the npc's facial mask group, type, and directives.

---

#### `String` npc.facialMaskDirectives()

Returns the npc's facial mask directives.

#### `void` npc.setFacialMaskDirectives(`String` facialMaskDirectives)

Sets the npc's facial mask directives.

---

#### `Color` npc.favoriteColor()

Returns the npc's favorite color.
It is used for the beam shown when wiring, placing, and highlighting with beam-tools (Matter Manipulator).

#### `void` npc.setFavoriteColor(`Color` color)

Sets the npc's favorite color. **color** can have an optional fourth value for transparency.

---
