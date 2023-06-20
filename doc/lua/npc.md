# npc

The `npc` table is for functions relating directly to the current npc. It is available only in NPC scripts.

---

#### `Vec2F` npc.toAbsolutePosition(`Vec2F` offset)

Returns the specified local position in world space.

---

#### `String` npc.species()

Returns the species of the npc.

---

#### `String` npc.gender()

Returns the gender of the npc

---

#### `Json` npc.humanoidIdentity()

Returns the specific humanoid identity of the npc, containing information such as hair style and idle pose.

---

#### `String` npc.npcType()

Returns the npc type of the npc.

---

#### `uint64_t` npc.seed()

Returns the seed used to generate this npc.

---

#### `float` npc.level()

Returns the level of the npc.

---

#### `List<String>` npc.dropPools()

Returns the list of treasure pools that will spawn when the npc dies.

---

#### `void` npc.setDropPools(`List<String>` pools)

Sets the list of treasure pools that will spawn when the npc dies.

---

#### `float` npc.energy()

Returns the current energy of the npc. Same as `status.resource("energy")`

---

#### `float` npc.maxEnergy()

Returns the current energy of the npc. Same as `status.maxResource("energy")`

---

#### `bool` npc.say(`String` line, [`Map<String,String>` tags], [`Json` config])

Makes the npc say a string. Optionally pass in tags to replace text tags. Optionally give config options for the chat message.

Returns whether the chat message was successfully added.

Available options:
```
{
  drawBorder = true,
  fontSize = 8,
  color = {255, 255, 255},
  sound = "/sfx/humanoid/avian_chatter_male1.ogg"
}
```

---

#### `bool` npc.sayPortrait(`String` line, `String` portrait, [`Map<String,String>` tags], [`Json` config])

Makes the npc say a line, with a portrait chat bubble. Optionally pass in tags to replace text tags. Optionally give config options for the chat message.
Returns whether the chat message was successfully added.

Available options:
```
{
  drawMoreIndicator = true,
  sound = "/sfx/humanoid/avian_chatter_male1.ogg"
}
```

---

#### `void` npc.emote(`String` emote)

Makes the npc show a facial emote.

---

#### `void` npc.dance(`String` dance)

Sets the current dance for the npc. Dances are defined in .dance files.

---

#### `void` npc.setInteractive(`bool` interactive)

Sets whether the npc should be interactive.

---

#### `bool` npc.setLounging(`EntityId` loungeable, [`size_t` anchorIndex])

Sets the npc to lounge in a loungeable. Optionally specify which anchor (seat) to use.
Returns whether the npc successfully lounged.

---

#### `void` npc.resetLounging()

Makes the npc stop lounging.

---

#### `bool` npc.isLounging()

Returns whether the npc is currently lounging.

---

#### `Maybe<EntityId>` npc.loungingIn()

Returns the EntityId of the loungeable the NPC is currently lounging in. Returns nil if not lounging.

---

#### `void` npc.setOfferedQuests(`JsonArray` quests)

Sets the list of quests the NPC will offer.

---

#### `void` npc.setTurnInQuests(`JsonArray` quests)

Sets the list of quests the played can turn in at this npc.

---

#### `bool` npc.setItemSlot(`String` slot, `ItemDescriptor` item)

Sets the specified item slot to contain the specified item.

Possible equipment items slots:
* head
* headCosmetic
* chest
* chestCosmetic
* legs
* legsCosmetic
* back
* backCosmetic
* primary
* alt

---

#### `ItemDescriptor` npc.getItemSlot(`String` slot)

Returns the item currently in the specified item slot.

---

#### `void` npc.disableWornArmor(`bool` disable)

Set whether the npc should not gain status effects from the equipped armor. Armor will still be visually equipped.

---

#### `void` npc.beginPrimaryFire()

Toggles `on` firing the item equipped in the `primary` item slot.

---

#### `void` npc.beginAltFire()

Toggles `on` firing the item equipped in the `alt` item slot.

---

#### `void` npc.endPrimaryFire()

Toggles `off` firing the item equipped in the `primary` item slot.

---

#### `void` npc.endAltFire()

Toggles `off` firing the item equipped in the `alt` item slot.

---

#### `void` npc.setShifting(`bool` shifting)

Sets whether tools should be used as though shift is held.

---

#### `void` npc.setDamageOnTouch(`bool` enabled)

Sets whether damage on touch should be enabled.

---

#### `Vec2F` npc.aimPosition()

Returns the current aim position in world space.

---

#### `void` npc.setAimPosition(`Vec2F` position)

Sets the aim position in world space.

---

#### `void` npc.setDeathParticleBurst(`String` emitter)

Sets a particle emitter to burst when the npc dies.

---

#### `void` npc.setStatusText(`String` status)

Sets the text to appear above the npc when it first appears on screen.

---

#### `void` npc.setDisplayNametag(`bool` display)

Sets whether the nametag should be displayed above the NPC.

---

#### `void` npc.setPersistent(`bool` persistent)

Sets whether this npc should persist after being unloaded.

---

#### `void` npc.setKeepAlive(`bool` keepAlive)

Sets whether to keep this npc alive. If true, the npc will never be unloaded as long as the world is loaded.

---

#### `void` npc.setDamageTeam(`Json` damageTeam)

Sets a damage team for the npc in the format: `{type = "enemy", team = 2}`

---

#### `void` npc.setAggressive(`bool` aggressive)

Sets whether the npc should be flagged as aggressive.

---

#### `void` npc.setUniqueId(`String` uniqueId)

Sets a unique ID for this npc that can be used to access it. A unique ID has to be unique for the world the npc is on, but not universally unique.
