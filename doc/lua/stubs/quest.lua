---@meta

--- quest API
---@class quest
quest = {}

--- Returns the current state of the quest. Possible states: * "New" * "Offer" * "Active" * "Complete" * "Failed" ---
---@return string
function quest.state() end

--- Immediately completes the quest. ---
---@return void
function quest.complete() end

--- Immediately fails the quest. ---
---@return void
function quest.fail() end

--- Sets whether the quest can be turned in. ---
---@param turnIn boolean
---@return void
function quest.setCanTurnIn(turnIn) end

--- Returns the quest id. ---
---@return string
function quest.questId() end

--- Returns the ID of the template used to make this quest. ---
---@return string
function quest.templateId() end

--- Returns the seed used to generate the quest. ---
---@return uint64_t
function quest.seed() end

--- Returns the quest descriptor including parameters. ---
---@return Json
function quest.questDescriptor() end

--- Returns the quest arc descriptor. ---
---@return Json
function quest.questArcDescriptor() end

--- Returns the quest arc position. (?) ---
---@return Vec2F
function quest.questArcPosition() end

--- Returns the world id for the quest arc. ---
---@return string
function quest.worldId() end

--- Returns the server uuid for the quest. ---
---@return string
function quest.serverUuid() end

--- Returns all quest parameters. ---
---@return QuestParameters
function quest.parameters() end

--- Sets a quest parameter. ---
---@param name string
---@param value Json
---@return void
function quest.setParameter(name, value) end

--- Set a list of quest parameters to use as custom indicators. Example: ``` -- define indicators local entityIndicator = { type = "entity", uniqueId = "techscientist" } local itemIndicator = { type = "item", item = "perfectlygenericitem" } local itemTagIndicator = { type = "itemTag", tag = "weapon" } local itemListIndicator = { type = "itemList", items = [ "perfectlygenericitem", "standingturret" ] } local monsterTypeIndicator = { type = "monsterType", typeName = "peblit" } -- set quest parameters for the indicators quest.setParameter("entity", entityIndicator) quest.setParameter("item", itemIndicator) quest.setParameter("itemTag", itemTagIndicator) quest.setParameter("itemList", itemListIndicator) quest.setParameter("monsterType", monsterTypeIndicator) -- add the set quest parameters to the indicators list quest.setIndicators({"entity", "item", "itemTag", "itemList", "monsterType"}) ``` ---
---@param indicators List<String>
---@return void
function quest.setIndicators(indicators) end

--- Set the objectives for the quest tracker. Objectives are in the format {text, completed} Example: ```lua quest.setObjectiveList({ {"Gather 4 cotton", true}, {"Eat 2 cotton", false} }) ``` ---
---@param objectives JsonArray
---@return void
function quest.setObjectiveList(objectives) end

--- Sets the progress amount of the quest tracker progress bar. Set nil to hide. Progress is from 0.0 to 1.0. ---
---@param progress number
---@return void
function quest.setProgress(progress) end

--- Set the angle of the quest tracker compass. Setting nil hides the compass. ---
---@param angle number
---@return void
function quest.setCompassDirection(angle) end

--- Sets the title of the quest in the quest log. ---
---@param title string
---@return void
function quest.setTitle(title) end

--- Set the text for the quest in the quest log. ---
---@param text string
---@return void
function quest.setText(text) end

--- Sets the text shown in the completion window when the quest is completed. ---
---@param completionText string
---@return void
function quest.setCompletionText(completionText) end

--- Sets the text shown in the completion window when the quest is failed. ---
---@param failureText string
---@return void
function quest.setFailureText(failureText) end

--- Sets a portrait to a list of drawables. ---
---@param portraitName string
---@param portrait JsonArray
---@return void
function quest.setPortrait(portraitName, portrait) end

--- Sets a portrait title. ---
---@param portraitName string
---@param title string
---@return void
function quest.setPortraitTitle(portraitName, title) end

--- Add an item to the reward pool.
---@param reward ItemDescriptor
---@return void
function quest.addReward(reward) end
