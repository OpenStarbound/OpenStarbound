# species

These are json files that have the `.species` extension, which define a humanoid species for players/npcs.


## `buildScripts`
Is an optional array of paths to lua scripts that are used to dyncamically change the player/npc's humanoidConfig based on their identity and humanoidParameters, much like how items can modify their resulting config with a build script.

`build(JsonObject identity, JsonObject humanoidParameters, JsonObject humanoidConfig, Variant<JsonObject, nil> npcHumanoidConfig)` will be invoked, and it must return a valid `JsonObject humanoidConfig` or the humanoid will fail to initalize correctly.

It is run whenever the player/npc initializes, changes species or imagePath in their identity, or the player/npc callback `refreshHumanoidParameters()` is used.

- `identity` is the specific identity of the player/npc it contains their customization data
- `humanoidParameters` are expected to be merged on top of `humanoidConfig`, they can also contain arbitrary data the build script can use to decide how to modify the output.
- `humanoidConfig` is the default humanoid config for the current species.
- `npcHumanoidConfig` is an optional argument that will be supplied when a NPC variant has its own unique config, such as the story NPCs. In all other cases it will be `nil`
