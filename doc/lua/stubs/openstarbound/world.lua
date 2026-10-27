---@meta

--- world API
---@class world
world = {}

--- Returns whether the script is running on the server or client. ---
---@return boolean
function world.isServer() end

--- Returns whether the script is running on the server or client. ---
---@return boolean
function world.isClient() end

--- Attempts a wire connection between the two objects positioned at `outputPosition` and `inputPosition`. The `outputIndex` and `inputIndex` parameters specify which output and input nodes to connect. --- The following additional world bindings are available only for scripts running on the client. ---
---@param outputPosition Vec2I
---@param outputIndex number
---@param inputPosition Vec2I
---@param inputIndex number
---@return void
function world.wire(outputPosition, outputIndex, inputPosition, inputIndex) end

--- Returns the entity ID of the player hosting the world. ---
---@return entityId
function world.mainPlayer() end

--- Returns the current cursor aim position of the specified entity. ---
---@param entityId entityId
---@return Vec2F
function world.entityAimPosition(entityId) end

--- Returns whether any players are in the world. --- The following additional world bindings are available only for scripts running on the server. ---
---@return boolean
function world.inWorld() end

--- Sets the amount of time to persist a ephemeral world when it is inactive. ---
---@param expiryTime number
---@return void
function world.setExpiryTime(expiryTime) end

--- Returns a `String` representation of the world's id. ---
---@return string
function world.id() end

--- Calls a function in the specified world script context. ---
---@param contextName string
---@param functionName string
---@param args LuaValue
---@return Maybe<LuaValue>
function world.callScriptContext(contextName, functionName, args) end

--- Sends a raw network packet to the specified client that is currently connected to this world. The `packetType` must be the name of a known packet in `Star::PacketTypeNames` (for example `"EntityInteractResult"`) and `packetData` must contain the JSON representation expected by `Star::createPacket` for that packet. Returns `true` when the packet is queued successfully. ---
---@param clientId ConnectionId
---@param packetType string
---@param packetData Json
---@return boolean
function world.sendPacket(clientId, packetType, packetData) end

--- Attempts to replace existing materials with the specified material in the specified positions and layer. Positions with no preexisting material will be skipped. Returns `true` if the placement succeeds and `false` otherwise. ---
---@param positions List<Vec2I>
---@param layerName string
---@param materialName string
---@param hueShift number
---@param enableDrops boolean
---@return boolean
function world.replaceMaterials(positions, layerName, materialName, hueShift, enableDrops) end

--- Identical to world.replaceMaterials but applies to tiles in a circular radius around the specified center point.
---@param center Vec2F
---@param radius number
---@param layerName string
---@param materialName string
---@param hueShift number
---@param enableDrops boolean
---@return boolean
function world.replaceMaterialArea(center, radius, layerName, materialName, hueShift, enableDrops) end
