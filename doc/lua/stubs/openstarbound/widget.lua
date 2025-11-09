---@meta

--- widget API
---@class widget
widget = {}

--- Gets the hint text of a TextBoxWidget. ---
---@param widgetName string
---@return string
function widget.getHint(widgetName) end

--- Sets the hint text of a TextBoxWidget. ---
---@param widgetName string
---@param hint string
---@return void
function widget.setHint(widgetName, hint) end

--- Gets the cursor position of a TextBoxWidget. ---
---@param widgetName string
---@return Int
function widget.getCursorPosition(widgetName) end

--- Sets the cursor position of a TextBoxWidget. --- ### SliderBarWidget helpers
---@param widgetName string
---@param cursorPosition number
---@return void
function widget.setCursorPosition(widgetName, cursorPosition) end

--- Sets the allowed range for a slider, optionally overriding the step used when the slider value is changed. The step defaults to `1` when omitted. --- ### ImageStretchWidget helpers
---@param widgetName string
---@param minimum number
---@param maximum number
---@param step number
---@return void
function widget.setSliderRange(widgetName, minimum, maximum, step) end

--- Sets the full image set of a ImageStretchWidget. ```lua { ["begin"] = "image.png", ["inner"] = "image.png", ["end"] = "image.png" } ``` --- ### FlowLayout helpers
---@param widgetName string
---@param imageStretchSet Json
---@return void
function widget.setImageStretchSet(widgetName, imageStretchSet) end

--- Appends a new ImageWidget to the specified FlowLayout widget, using `childName` as the new widget's identifier within the layout.
---@param widgetName string
---@param childName string
---@param imagePath string
---@return void
function widget.addFlowImage(widgetName, childName, imagePath) end
