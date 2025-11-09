---@meta

--- widget API
---@class widget
widget = {}

--- Plays a sound. ---
---@param audio string
---@param loops number
---@param volume number
---@return void
function widget.playSound(audio, loops, volume) end

--- Returns the position of a widget. ---
---@param widgetName string
---@return Vec2I
function widget.getPosition(widgetName) end

--- Sets the position of a widget. ---
---@param widgetName string
---@param position Vec2I
---@return void
function widget.setPosition(widgetName, position) end

--- Returns the size of a widget. ---
---@param widgetName string
---@return Vec2I
function widget.getSize(widgetName) end

--- Sets the size of a widget. ---
---@param widgetName string
---@param size Vec2I
---@return void
function widget.setSize(widgetName, size) end

--- Sets the visibility of a widget. ---
---@param widgetName string
---@param visible boolean
---@return void
function widget.setVisible(widgetName, visible) end

--- Returns whether the widget is visible. ---
---@param widgetName string
---@return void
function widget.active(widgetName) end

--- Sets focus on the specified widget. ---
---@param widgetName string
---@return void
function widget.focus(widgetName) end

--- Returns whether the specified widget is currently focused. ---
---@param widgetName string
---@return void
function widget.hasFocus(widgetName) end

--- Unsets focus on the specified focused widget. ---
---@param widgetName string
---@return void
function widget.blur(widgetName) end

--- Returns the arbitrary data value set for the widget. ---
---@param widgetName string
---@return Json
function widget.getData(widgetName) end

--- Sets arbitrary data for the widget. ---
---@param widgetName string
---@param data Json
---@return void
function widget.setData(widgetName, data) end

--- Returns the full name for any widget at screenPosition. ---
---@param screenPosition Vec2I
---@return string
function widget.getChildAt(screenPosition) end

--- Returns whether the widget contains the specified screenPosition. ---
---@param widgetName string
---@param screenPosition Vec2I
---@return boolean
function widget.inMember(widgetName, screenPosition) end

--- Creates a new child widget with the specified config and adds it to the specified widget, optionally with the specified name. ---
---@param widgetName string
---@param childConfig Json
---@param childName string
---@return void
function widget.addChild(widgetName, childConfig, childName) end

--- Removes all child widgets of the specified widget. ---
---@param widgetName string
---@return void
function widget.removeAllChildren(widgetName) end

--- Removes the specified child widget from the specified widget. --- ## Widget specific callbacks These callbacks only work for some widget types. ---
---@param widgetName string
---@param childName string
---@return void
function widget.removeChild(widgetName, childName) end

--- Returns the text set in a TextBoxWidget. ---
---@param widgetName string
---@return string
function widget.getText(widgetName) end

--- Sets the text of: LabelWidget, ButtonWidget, TextBoxWidget ---
---@param widgetName string
---@param text string
---@return void
function widget.setText(widgetName, text) end

--- Sets the font color of: LabelWidget, ButtonWidget, TextBoxWidget ---
---@param widgetName string
---@param color Color
---@return void
function widget.setFontColor(widgetName, color) end

--- Sets the image of an ImageWidget. ---
---@param widgetName string
---@param imagePath string
---@return void
function widget.setImage(widgetName, imagePath) end

--- Sets the scale of an ImageWidget. ---
---@param widgetName string
---@param imageScale number
---@return void
function widget.setImageScale(widgetName, imageScale) end

--- Sets the rotation of an ImageWidget. ---
---@param widgetName string
---@param imageRotation number
---@return void
function widget.setImageRotation(widgetName, imageRotation) end

--- Sets whether the ButtonWidget should be enabled. ---
---@param widgetName string
---@param enabled boolean
---@return void
function widget.setButtonEnabled(widgetName, enabled) end

--- Sets the baseImage of a ButtonWidget. ---
---@param widgetName string
---@param baseImage string
---@return void
function widget.setButtonImage(widgetName, baseImage) end

--- Sets the full image set of a ButtonWidget. ``` { base = "image.png", hover = "image.png", pressed = "image.png", disabled = "image.png", } ``` ---
---@param widgetName string
---@param imageSet Json
---@return void
function widget.setButtonImages(widgetName, imageSet) end

--- Similar to widget.setButtonImages, but sets the images used for the checked state of a checkable ButtonWidget. ---
---@param widgetName string
---@param imageSet Json
---@return void
function widget.setButtonCheckedImages(widgetName, imageSet) end

--- Sets the overlay image of a ButtonWidget. ---
---@param widgetName string
---@param overlayImage string
---@return void
function widget.setButtonOverlayImage(widgetName, overlayImage) end

--- Returns whether the ButtonWidget is checked. ---
---@param widgetName string
---@return boolean
function widget.getChecked(widgetName) end

--- Sets whether a ButtonWidget is checked ---
---@param widgetName string
---@param checked boolean
---@return void
function widget.setChecked(widgetName, checked) end

--- Returns the index of the selected option in a ButtonGroupWidget. ---
---@param widgetName string
---@return number
function widget.getSelectedOption(widgetName) end

--- Returns the data of the selected option in a ButtonGroupWidget. Nil if no option is selected. ---
---@param widgetName string
---@return number
function widget.getSelectedData(widgetName) end

--- Sets the selected option index of a ButtonGroupWidget. ---
---@param widgetName string
---@param index number
---@return void
function widget.setSelectedOption(widgetName, index) end

--- Sets whether a ButtonGroupWidget option should be enabled. ---
---@param widgetName string
---@param index number
---@param enabled boolean
---@return void
function widget.setOptionEnabled(widgetName, index, enabled) end

--- Sets whether a ButtonGroupWidget option should be visible. ---
---@param widgetName string
---@param index number
---@return void
function widget.setOptionVisible(widgetName, index) end

--- Sets the progress of a ProgressWidget. Value should be between 0.0 and 1.0. ---
---@param widgetName string
---@param value number
---@return void
function widget.setProgress(widgetName, value) end

--- Sets whether the SliderBarWidget should be enabled. ---
---@param widgetName string
---@param enabled boolean
---@return void
function widget.setSliderEnabled(widgetName, enabled) end

--- Gets the current value of a SliderBarWidget. ---
---@param widgetName string
---@return number
function widget.getSliderValue(widgetName) end

--- Sets the current value of a SliderBarWidget. ---
---@param widgetName string
---@param newValue number
---@return void
function widget.setSliderValue(widgetName, newValue) end

--- Sets the minimum, maximum and (optionally) delta values of a SliderBarWidget. ---
---@param widgetName string
---@param newMin number
---@param newMax number
---@param newDelta number
---@return void
function widget.getSliderRange(widgetName, newMin, newMax, newDelta) end

--- Clears all items in a ListWidget. ---
---@param widgetName string
---@return void
function widget.clearListItems(widgetName) end

--- Adds a list item to a ListWidget using the configured template, and returns the name of the added list item. ---
---@param widgetName string
---@return string
function widget.addListItem(widgetName) end

--- Removes a list item from a ListWidget at a specific index. ---
---@param widgetName string
---@param at size_t
---@return void
function widget.removeListItem(widgetName, at) end

--- Returns the name of the currently selected widget in a ListWidget. ---
---@param widgetName string
---@return string
function widget.getListSelected(widgetName) end

--- Sets the selected widget of a ListWidget. ---
---@param widgetName string
---@param selected string
---@return void
function widget.setListSelected(widgetName, selected) end

--- Registers a member callback for a ListWidget's list items to use. ---
---@param widgetName string
---@param callbackName string
---@param callback LuaFunction
---@return void
function widget.registerMemberCallback(widgetName, callbackName, callback) end

--- Returns the full item bag contents of an ItemGridWidget. ---
---@param widgetName string
---@return ItemBag
function widget.itemGridItems(widgetName) end

--- Returns the descriptor of the item in the specified item slot widget. ---
---@param widgetName string
---@return ItemDescriptor
function widget.itemSlotItem(widgetName) end

--- Sets the item in the specified item slot widget. ---
---@param widgetName string
---@param itemDescriptor Json
---@return void
function widget.setItemSlotItem(widgetName, itemDescriptor) end

--- Sets the progress overlay on the item slot to the specified value (between 0 and 1). ---
---@param widgetName string
---@param progress number
---@return void
function widget.setItemSlotProgress(widgetName, progress) end

--- Binds the canvas widget with the specified name as userdata for easy access. The `CanvasWidget` has the following methods:
---@param widgetName string
---@return CanvasWidget
function widget.bindCanvas(widgetName) end

--- Global functions

--- Returns the size of the canvas.
---@return Vec2I
function size() end

--- Clears the canvas.
---@return void
function clear() end

--- Returns the mouse position relative to the canvas.
---@return Vec2I
function mousePosition() end

--- Draws an image to the canvas.
---@param image string
---@param position Vec2F
---@param scale number
---@param color Color
---@param centered boolean
---@return void
function drawImage(image, position, scale, color, centered) end

--- Draws an image to the canvas, centered on position, with slightly different options.
---@param image string
---@param position Vec2F
---@param scale Variant<Vec2F, float>
---@param color Color
---@param rotation number
---@return void
function drawImageDrawable(image, position, scale, color, rotation) end

--- Draws a rect section of a texture to a rect section of the canvas.
---@param texName string
---@param texCoords RectF
---@param screenCoords RectF
---@param color Color
---@return void
function drawImageRect(texName, texCoords, screenCoords, color) end

--- Draws an image tiled (and wrapping) within the specified screen area.
---@param image string
---@param offset Vec2F
---@param screenCoords RectF
---@param scale number
---@param color Color
---@return void
function drawTiledImage(image, offset, screenCoords, scale, color) end

--- Draws a line on the canvas.
---@param start Vec2F
---@param end Vec2F
---@param color Color
---@param lineWidth number
---@return void
function drawLine(start, end, color, lineWidth) end

--- Draws a filled rectangle on the canvas.
---@param rect RectF
---@param color Color
---@return void
function drawRect(rect, color) end

--- Draws a polygon on the canvas.
---@param poly PolyF
---@param color Color
---@param lineWidth number
---@return void
function drawPoly(poly, color, lineWidth) end

--- Draws a list of filled triangles to the canvas.
---@param triangles List<PolyF>
---@param color Color
---@return void
function drawTriangles(triangles, color) end

--- Draws text on the canvas. textPositioning is in the format: ```lua { position = {0, 0} horizontalAnchor = "left", -- left, mid, right verticalAnchor = "top", -- top, mid, bottom wrapWidth = nil -- wrap width in pixels or nil } ```
---@param text string
---@param textPositioning Json
---@param fontSize unsigned
---@param color Color
---@param lineSpacing number
---@param directives string
---@return void
function drawText(text, textPositioning, fontSize, color, lineSpacing, directives) end
