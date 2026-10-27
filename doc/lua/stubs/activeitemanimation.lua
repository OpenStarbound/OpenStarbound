---@meta

--- activeItemAnimation API
---@class activeItemAnimation
activeItemAnimation = {}

--- Returns the current entity position of the item's owner. ---
---@return Vec2F
function activeItemAnimation.ownerPosition() end

--- Returns the current world aim position of the item's owner. ---
---@return Vec2F
function activeItemAnimation.ownerAimPosition() end

--- Returns the current angle of the arm holding the item. ---
---@return number
function activeItemAnimation.ownerArmAngle() end

--- Returns the current facing direction of the item's owner. Will return 1 for right or -1 for left. ---
---@return number
function activeItemAnimation.ownerFacingDirection() end

--- Takes an input position (defaults to [0, 0]) relative to the item and returns a position relative to the owner entity. ---
---@param offset Vec2F
---@return Vec2F
function activeItemAnimation.handPosition(offset) end

--- Returns a transformation of the specified `Vec2F` parameter configured on the specified animation part, relative to the owner's position. ---
---@param partName string
---@param propertyName string
---@return Vec2F
function activeItemAnimation.partPoint(partName, propertyName) end

--- Returns a transformation of the specified `PolyF` parameter configured on the specified animation part, relative to the owner's position.
---@param partName string
---@param propertyName string
---@return PolyF
function activeItemAnimation.partPoly(partName, propertyName) end
