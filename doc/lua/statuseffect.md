# effect

The `effect` table relates to functions specifically for status effects.

---

#### `float` effect.duration()

Returns the remaining duration of the status effect.

---

#### `void` effect.modifyDuration(`float` duration)

Adds the specified duration to the current remaining duration.

---

#### `void` effect.expire()

Immediately expire the effect, setting the duration to 0.

---

#### `EntityId` effect.sourceEntity()

Returns the source entity id of the status effect, if any.

---

#### `void` effect.setParentDirectives(`String` directives)

Sets image processing directives for the entity the status effect is active on.

---

#### `Json` effect.getParameter(`String` name, `Json` def)

Returns the value associated with the parameter name in the effect configuration. If no value is set, returns the default specified.

---

#### `StatModifierGroupId` effect.addStatModifierGroup(`List<StatModifier>` modifiers)

Adds a new stat modifier group and returns the ID created for the group. Stat modifier groups will stay active until the effect expires.

Stat modifiers are of the format:

```lua
{
  stat = "health",

  amount = 50
  --OR baseMultiplier = 1.5
  --OR effectiveMultiplier = 1.5
}
```

---

#### `void` effect.setStatModifierGroup(`StatModifierGroupId`, groupId, `List<StatModifier>` modifiers)

Replaces the list of stat modifiers in a group with the specified modifiers.

---

#### `void` effect.removeStatModifierGroup(`StatModifierGroupId` groupId)

Removes the specified stat modifier group.
