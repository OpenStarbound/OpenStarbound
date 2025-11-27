---@meta

--- camera API
---@class camera
camera = {}

--- Returns the center world position of the camera. ---
---@return Vec2F
function camera.position() end

--- Returns the current pixel ratio (zoom level) of the camera. ---
---@return Float
function camera.pixelRatio() end

--- Sets the pixel ratio (zoom level) of the camera. If smooth is `true`, the camera will smoothly transition to the new zoom level. ---
---@param pixelRatio Float
---@param smooth Bool
---@return void
function camera.setPixelRatio(pixelRatio, smooth) end

--- Returns the screen size in pixels as a Vec2U. ---
---@return Vec2U
function camera.screenSize() end

--- Returns the world rectangle visible on screen. ---
---@return RectF
function camera.worldScreenRect() end

--- Returns the tile rectangle visible on screen. ---
---@return RectI
function camera.worldTileRect() end

--- Returns the minimum tile size on screen. ---
---@return Vec2F
function camera.tileMinScreen() end

--- Converts a screen position to a world position. ---
---@param screenPosition Vec2F
---@return Vec2F
function camera.screenToWorld(screenPosition) end

--- Converts a world position to a screen position.
---@param worldPosition Vec2F
---@return Vec2F
function camera.worldToScreen(worldPosition) end
