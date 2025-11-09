---@meta

--- item API
---@class item
item = {}

--- Returns the name of the item. ---
---@return string
function item.name() end

--- Returns the stack count of the item. ---
---@return size_t
function item.count() end

--- Sets the item count. Returns any overflow. ---
---@param count size_t
---@return size_t
function item.setCount(count) end

--- Returns the max number of this item that will fit in a stack. ---
---@return size_t
function item.maxStack() end

--- Returns whether the item matches the specified item. If exactMatch is `true` then both the items' names and parameters are compared, otherwise only the items' names. ---
---@param desc ItemDescriptor
---@param exactMatch boolean
---@return boolean
function item.matches(desc, exactMatch) end

--- Consumes items from the stack. Returns whether the full count was successfuly consumed. ---
---@param count size_t
---@return boolean
function item.consume(count) end

--- Returns whether the item stack is empty. ---
---@return boolean
function item.empty() end

--- Returns an item descriptor for the item. ---
---@return ItemDescriptor
function item.descriptor() end

--- Returns the description for the item. ---
---@return string
function item.description() end

--- Returns the short description for the item. ---
---@return string
function item.friendlyName() end

--- Returns the rarity for the item. * 0 = common * 1 = uncommon * 2 = rare * 3 = legendary * 4 = essential ---
---@return number
function item.rarity() end

--- Returns the rarity as a string. ---
---@return string
function item.rarityString() end

--- Returns the item price. ---
---@return size_t
function item.price() end

--- Returns the item fuel amount. ---
---@return unsigned
function item.fuelAmount() end

--- Returns a list of the item's icon drawables. ---
---@return Json
function item.iconDrawables() end

--- Returns a list of the item's itemdrop drawables. ---
---@return Json
function item.dropDrawables() end

--- Returns the item's configured large image, if any. ---
---@return string
function item.largeImage() end

--- Returns the item's tooltip kind. ---
---@return string
function item.tooltipKind() end

--- Returns the item's category ---
---@return string
function item.category() end

--- Returns the item's pickup sound. ---
---@return string
function item.pickupSound() end

--- Returns whether the item is two handed. ---
---@return boolean
function item.twoHanded() end

--- Returns the items's time to live. ---
---@return number
function item.timeToLive() end

--- Returns a list of the blueprints learned on picking up this item. ---
---@return Json
function item.learnBlueprintsOnPickup() end

--- Returns whether the set of item tags for this item contains the specified tag. ---
---@param itemTag string
---@return boolean
function item.hasItemTag(itemTag) end

--- Returns a list of quests acquired on picking up this item.
---@return Json
function item.pickupQuestTemplates() end
