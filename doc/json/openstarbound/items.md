Not to be confused with generic items (items with the `.item` extension). This is for parameters shared amongst most or all item types.

## `String` rarityBorder (All item types)
Overrides the rarity border image. If an absolute path is not provided, the file at `/interface/inventory/itemborder<rarityBorder>.png` is used. Cosmetic only; the item's actual `rarity` is unchanged.
Note that the rarity name shown in tooltips can be changed via the `rarityLabel` of an item's `tooltipFields`.
