# quest

The `quest` table contains functions relating directly to the quest whose script its run in.

---

#### `String` quest.state()

Returns the current state of the quest.

Possible states:
* "New"
* "Offer"
* "Active"
* "Complete"
* "Failed"

---

#### `void` quest.complete()

Immediately completes the quest.

---

#### `void` quest.fail()

Immediately fails the quest.

---

#### `void` quest.setCanTurnIn(`bool` turnIn)

Sets whether the quest can be turned in.

---

#### `String` quest.questId()

Returns the quest id.

---

#### `String` quest.templateId()

Returns the ID of the template used to make this quest.

---

#### `uint64_t` quest.seed()

Returns the seed used to generate the quest.

---

#### `Json` quest.questDescriptor()

Returns the quest descriptor including parameters.

---

#### `Json` quest.questArcDescriptor()

Returns the quest arc descriptor.

---

#### `Vec2F` quest.questArcPosition()

Returns the quest arc position. (?)

---

#### `String` quest.worldId()

Returns the world id for the quest arc.

---

#### `String` quest.serverUuid()

Returns the server uuid for the quest.

---

#### `QuestParameters` quest.parameters()

Returns all quest parameters.

---

#### `void` quest.setParameter(`String` name, `Json` value)

Sets a quest parameter.

---

#### `void` quest.setIndicators(`List<String>` indicators)

Set a list of quest parameters to use as custom indicators.

Example:
```
-- define indicators
local entityIndicator = {
  type = "entity",
  uniqueId = "techscientist"
}
local itemIndicator = {
  type = "item",
  item = "perfectlygenericitem"
}
local itemTagIndicator = {
  type = "itemTag",
  tag = "weapon"
}
local itemListIndicator = {
  type = "itemList",
  items = [ "perfectlygenericitem", "standingturret" ]
}
local monsterTypeIndicator = {
  type = "monsterType",
  typeName = "peblit"
}

-- set quest parameters for the indicators
quest.setParameter("entity", entityIndicator)
quest.setParameter("item", itemIndicator)
quest.setParameter("itemTag", itemTagIndicator)
quest.setParameter("itemList", itemListIndicator)
quest.setParameter("monsterType", monsterTypeIndicator)

-- add the set quest parameters to the indicators list
quest.setIndicators({"entity", "item", "itemTag", "itemList", "monsterType"})
```

---

#### `void` quest.setObjectiveList(`JsonArray` objectives)

Set the objectives for the quest tracker. Objectives are in the format {text, completed}

Example:
```lua
quest.setObjectiveList({
  {"Gather 4 cotton", true},
  {"Eat 2 cotton", false}
})
```

---

#### `void` quest.setProgress(`float` progress)

Sets the progress amount of the quest tracker progress bar. Set nil to hide. Progress is from 0.0 to 1.0.

---

#### `void` quest.setCompassDirection(`float` angle)

Set the angle of the quest tracker compass. Setting nil hides the compass.

---

#### `void` quest.setTitle(`String` title)

Sets the title of the quest in the quest log.

---

#### `void` quest.setText(`String` text)

Set the text for the quest in the quest log.

---

#### `void` quest.setCompletionText(`String` completionText)

Sets the text shown in the completion window when the quest is completed.

---

#### `void` quest.setFailureText(`String` failureText)

Sets the text shown in the completion window when the quest is failed.

---

#### `void` quest.setPortrait(`String` portraitName, `JsonArray` portrait)

Sets a portrait to a list of drawables.

---

#### `void` quest.setPortraitTitle(`String` portraitName, `String` title)

Sets a portrait title.

---

#### `void` quest.addReward(`ItemDescriptor` reward)

Add an item to the reward pool.