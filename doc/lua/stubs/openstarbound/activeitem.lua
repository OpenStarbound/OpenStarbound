---@meta

--- activeItem API
---@class activeItem
activeItem = {}

--- If **icon** is a `String` it will set the item's secondary icon to the image at it's path, this path can be local to the item's directory, or a global path. If **icon** is a `List<Drawable>` it will use those drawbles as the item's secondary icon. Any image drawables can have their path be local to the item's directory or global as well. Secondary icons are shown in the right hand slot of two handed items. ---
---@return void
function activeItem.setSecondaryIcon() end

--- Sets the item's description ---
---@param description string
---@return void
function activeItem.setDescription(description) end

--- Sets the item's short description ---
---@param shortDescription string
---@return void
function activeItem.setShortDescription(shortDescription) end

--- Triggers the item to fire if it is currently ready. If **fireMode** is specified it must be either `"Primary"` or `"Alt"`, otherwise the primary fire mode is used. ---
---@param fireMode string
---@return void
function activeItem.fire(fireMode) end

--- Immediately starts the item's cooldown using its current fire time. ---
---@return void
function activeItem.triggerCooldown() end

--- Sets the current cooldown timer to **cooldownTime** seconds without modifying the item's default cooldown length. ---
---@param cooldownTime number
---@return void
function activeItem.setCooldown(cooldownTime) end

--- Clears the current cooldown timer, making the item ready to fire again. ---
---@return void
function activeItem.endCooldown() end

--- Returns the item's configured default cooldown duration in seconds. ---
---@return number
function activeItem.cooldownTime() end

--- Returns the value of the fireable parameter **name** from the item's configuration, or **default** when the parameter is not defined. ---
---@param name string
---@param default Json
---@return Json
function activeItem.fireableParam(name, default) end

--- Returns whether the item can currently begin firing. ---
---@return boolean
function activeItem.ready() end

--- Returns whether the item is currently firing. ---
---@return boolean
function activeItem.firing() end

--- Returns whether the item is winding up in preparation to fire. ---
---@return boolean
function activeItem.windingUp() end

--- Returns whether the item is cooling down after firing. ---
---@return boolean
function activeItem.coolingDown() end

--- Returns whether the item's owner currently has a full energy bar. ---
---@return boolean
function activeItem.ownerFullEnergy() end

--- Returns whether the item's owner has any energy available. ---
---@return boolean
function activeItem.ownerEnergy() end

--- Returns whether the item's owner currently has their energy locked. ---
---@return boolean
function activeItem.ownerEnergyLocked() end

--- Attempts to consume **amount** energy from the item's owner and returns whether the energy was successfully consumed. ---
---@param amount number
---@return boolean
function activeItem.ownerConsumeEnergy(amount) end
