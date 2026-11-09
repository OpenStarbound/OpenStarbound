# `/itemdrop.config`

Keys that OpenStarbound adds to the vanilla item drop config. Every one defaults to the vanilla behavior, so the base game is vanilla-like and mods (`smoothItemPickup`, `rarityBeam`) opt in.

| Key | Default | Meaning |
|---|---|---|
| `drawRarityBeam` | `false` | Draw a pillar of light, colored by rarity, above a dropped item. |
| `takenVelocityRamp` | `false` | An item flying to its owner scales its speed down within one tile of them, instead of flying at full speed in a straight line. |
| `takenAddsOwnerVelocity` | `false` | The flying item adds the velocity of the player who takes it. |
| `takenRenderLayer` | `"ItemDrop"` | Render layer of an item that was taken (any name `parseRenderLayer` knows, such as `"ForegroundTile"`). |
| `overheadTime` | `0` | Seconds an item first floats above its owner before flying in. |
| `overheadApproach` | `2000` | How fast the item reaches its overhead position. |
| `overheadRandomizedDistance` | `0.25` | Random horizontal offset of the overhead position. |

`pickupDistance` and `afterTakenLife` are the vanilla keys; OpenStarbound's `smoothItemPickup` lowers them.

Taken items are moved by the entity that is master of the drop (the server), so a mod that changes the movement has to be on the server as well. Rendering keys only matter on the client.
