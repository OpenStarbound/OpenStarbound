---@meta

--- entity API
---@class entity
entity = {}

--- Returns the id number of the entity. ---
---@return EntityId
function entity.id() end

--- Returns a table of the entity's damage team type and team number. Ex: {type = "enemy", team = 0} ---
---@return LuaTable
function entity.damageTeam() end

--- Returns whether the provided entity is a valid target of the current entity. An entity is a valid target if they can be damaged, and in the case of monsters and NPCs if they are aggressive. ---
---@param entityId EntityId
---@return boolean
function entity.isValidTarget(entityId) end

--- Returns the vector distance from the current entity to the provided entity. ---
---@param entityId EntityId
---@return Vec2F
function entity.distanceToEntity(entityId) end

--- Returns whether the provided entity is in line of sight of the current entity. ---
---@param entityId EntityId
---@return boolean
function entity.entityInSight(entityId) end

--- Returns the position of the current entity. ---
---@return Vec2F
function entity.position() end

--- Returns the  type of the current entity. ---
---@return string
function entity.entityType() end

--- Returns the unique ID of the entity. Returns nil if there is no unique ID. ---
---@return string
function entity.uniqueId() end

--- Returns `true` if the entity is persistent (will be saved to disk on sector unload) or `false` otherwise.
---@return boolean
function entity.persistent() end
