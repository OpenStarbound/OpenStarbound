---@meta

--- npc API
---@class npc
npc = {}

--- Sets the specific humanoid identity of the npc. Implicitly calls `npc.refreshHumanoidParameters()` if species or imagePath is changed. ---
---@param humanoidIdentity Json
---@return void
function npc.setHumanoidIdentity(humanoidIdentity) end

--- Returns the npc's name. ---
---@return string
function npc.name() end

--- Sets the npc's name. ---
---@param name string
---@return void
function npc.setName(name) end

--- Returns the npc's description. ---
---@return string
function npc.description() end

--- Sets the npc's description. The new description will not be networked until the npc is re-loaded. ---
---@param description string
---@return void
function npc.setDescription(description) end

--- Refreshes the active humanoid parameters by re-building the current humanoid instance. This must be called when certain values in the humanoid config would be changed by a parameter, such as the animationConfig and movementParameters. This is implicitly called whenever the current humanoid identity's species or imagePath changes. This has to network the humanoid identity and humanoid parameters as intialization arguments for the new instance, so only use it when necessary. ---
---@return void
function npc.refreshHumanoidParameters() end

--- Sets a parameter that overwrites a value in the humanoid config. ---
---@param key string
---@return void
function npc.setHumanoidParameter(key) end

--- Replaces the current table of humanoid parameters that overwrite values in the humanoid config. ---
---@param parameters JsonObject
---@return void
function npc.setHumanoidParameters(parameters) end

--- Gets a humanoid parameter. ---
---@param key string
---@return Maybe<Json>
function npc.getHumanoidParameter(key) end

--- Gets a table of all the humanoid parameters. ---
---@return JsonObject
function npc.getHumanoidParameters() end

--- Gets the active humanoid config. If withOverrides is true, returns it with the overrides applied by armor. The humanoid config controls how a the npc is animated, as well as their movement properties. ---
---@param withOverrides boolean
---@return JsonObject
function npc.humanoidConfig(withOverrides) end

--- Sets the npc's species. Must be a valid species. Implicitly calls `npc.refreshHumanoidParameters()` if species is changed. ---
---@param species string
---@return void
function npc.setSpecies(species) end

--- Sets the npc's gender. ---
---@param gender string
---@return void
function npc.setGender(gender) end

--- If the npc has a custom humanoid image path set, returns it. otherwise, returns `nil`. ---
---@return string
function npc.imagePath() end

--- Sets the npc's image path. Specify `nil` to remove the image path. Implicitly calls `npc.refreshHumanoidParameters()` if imagePath is changed. ---
---@param imagePath string
---@return void
function npc.setImagePath(imagePath) end

--- Returns the npc's personality as a `table` containing a `String` idle, `String` armIdle, `Vec2F` headOffset and `Vec2F` armOffset. ---
---@return Personality
function npc.personality() end

--- Sets the npc's personality. The **personality** must be a `table` containing at least one value as returned by `npc.personality()`. ---
---@param personality Personality
---@return void
function npc.setPersonality(personality) end

--- Returns the npc's body directives. ---
---@return string
function npc.bodyDirectives() end

--- Sets the npc's body directives. ---
---@param bodyDirectives string
---@return void
function npc.setBodyDirectives(bodyDirectives) end

--- Returns the npc's emote directives. ---
---@return string
function npc.emoteDirectives() end

--- Sets the npc's emote directives. ---
---@param emoteDirectives string
---@return void
function npc.setEmoteDirectives(emoteDirectives) end

--- Sets the npc's hair group, type, and directives. ---
---@param hairGroup string
---@param hairType string
---@param hairDirectives string
---@return void
function npc.setHair(hairGroup, hairType, hairDirectives) end

--- Returns the npc's hair group. ---
---@return string
function npc.hairGroup() end

--- Sets the npc's hair group. ---
---@param hairGroup string
---@return void
function npc.setHairGroup(hairGroup) end

--- Returns the npc's hair type. ---
---@return string
function npc.hairType() end

--- Sets the npc's hair type. ---
---@param hairType string
---@return void
function npc.setHairType(hairType) end

--- Returns the npc's hair directives. ---
---@return string
function npc.hairDirectives() end

--- Sets the npc's hair directives. ---
---@param hairDirectives string
---@return void
function npc.setHairDirectives(hairDirectives) end

--- Returns the npc's facial hair type. Same as npc.facialHairType? ---
---@return string
function npc.facialHair() end

--- Sets the npc's facial hair group, type, and directives. ---
---@param facialHairGroup string
---@param facialHairType string
---@param facialHairDirectives string
---@return void
function npc.setFacialHair(facialHairGroup, facialHairType, facialHairDirectives) end

--- Returns the npc's facial hair type. ---
---@return string
function npc.facialHairType() end

--- Sets the npc's facial hair type. ---
---@param facialHairType string
---@return void
function npc.setFacialHairType(facialHairType) end

--- Returns the npc's facial hair group. ---
---@return string
function npc.facialHairGroup() end

--- Sets the npc's facial hair group. ---
---@param facialHairGroup string
---@return void
function npc.setFacialHairGroup(facialHairGroup) end

--- Returns the npc's facial hair directives. ---
---@return string
function npc.facialHairDirectives() end

--- Sets the npc's facial hair directives. ---
---@param facialHairDirectives string
---@return void
function npc.setFacialHairDirectives(facialHairDirectives) end

--- Returns the npc's facial mask group. ---
---@return string
function npc.facialMask() end

--- Sets the npc's facial mask group, type, and directives. ---
---@param facialMaskGroup string
---@param facialMaskType string
---@param facialMaskDirectives string
---@return void
function npc.setFacialMask(facialMaskGroup, facialMaskType, facialMaskDirectives) end

--- Returns the npc's facial mask directives. ---
---@return string
function npc.facialMaskDirectives() end

--- Sets the npc's facial mask directives. ---
---@param facialMaskDirectives string
---@return void
function npc.setFacialMaskDirectives(facialMaskDirectives) end

--- Returns the npc's favorite color. It is used for the beam shown when wiring, placing, and highlighting with beam-tools (Matter Manipulator). ---
---@return Color
function npc.favoriteColor() end

--- Sets the npc's favorite color. **color** can have an optional fourth value for transparency.
---@param color Color
---@return void
function npc.setFavoriteColor(color) end
