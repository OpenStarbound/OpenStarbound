---@meta

--- interface API
---@class interface
interface = {}

--- Queues a message popup at the bottom of the screen with an optional **cooldown** and **springState**. ---
---@param message string
---@param cooldown number
---@param springState number
---@return void
function interface.queueMessage(message, cooldown, springState) end

--- Sets the HUD's visibility. ---
---@param visible boolean
---@return void
function interface.setHudVisible(visible) end

--- Returns the HUD's visibility. ---
---@return boolean
function interface.hudVisible() end

--- Binds a registered pane (defined in `/source/frontend/StarMainInterfaceTypes`) to a Lua value, which can then call widget functions on that pane. EscapeDialog<br> Inventory<br> Codex<br> Cockpit<br> Tech<br> Songbook<br> Ai<br> Popup<br> Confirmation<br> JoinRequest<br> Options<br> QuestLog<br> ActionBar<br> TeamBar<br> StatusPane<br> Chat<br> WireInterface<br> PlanetText<br> RadioMessagePopup<br> CraftingPlain<br> QuestTracker<br> MmUpgrade<br> Collections<br> ---
---@param paneName string
---@return PaneId
function interface.bindRegisteredPane(paneName) end

--- Displays a registered pane. ---
---@param paneName string
---@return void
function interface.displayRegisteredPane(paneName) end

--- Binds the canvas widget on the main interface with the specified name as userdata for easy access. The `CanvasWidget` has the same methods as described in `widget.md`. - **ignoreInterfaceScale** is used to ignore the current interface scaling and bind the canvas with the screen size. ---
---@param name string
---@param ignoreInterfaceScale boolean
---@return CanvasWidget
function interface.bindCanvas(name, ignoreInterfaceScale) end

--- Returns the scale used for interfaces.
---@return number
function interface.scale() end
