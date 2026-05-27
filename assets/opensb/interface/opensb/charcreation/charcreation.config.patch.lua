function patch(config)
  engineEditor = assets.json("/interface/windowconfig/charcreation.config")
  config.speciesOrdering = engineEditor.speciesOrdering
--  local oldHeader = config.gui.background.fileHeader --debug grabby part

  local validCallbacks = {}
  for i = 1, #config.scriptWidgetCallbacks do
    validCallbacks[config.scriptWidgetCallbacks[i]] = true
  end
  sb.logInfo(sb.print(config.scriptWidgetCallbacks))
  sb.logInfo(sb.print(validCallbacks))

  local CV = {
    "speciesOrdering",
    gui = {
      "background",
      species = {
        "buttons"
      }
    }
  }
--[[
  for k, v in pairs(CV) do
    if type(k) == "string" then
      config[k] = engineEditor[k]
    end
  end
--]]

--[[
  config.gui.background = engineEditor.paneLayout.background
  config.gui.species.buttons = engineEditor.paneLayout.species.buttons
]]

  for k, v in pairs(engineEditor.paneLayout) do
    if not config.gui[k] then
--    sb.logInfo("Adding: " ..k)
--    sb.logInfo("Adding: " ..sb.printJson(v))
      if v.callback and not validCallbacks[v.callback] then
--      sb.logInfo("Could not find" .. v.callback)
        v.callback = "null"
      end
      config.gui[k] = v
    else
      sb.logInfo(k)
    end
  end

  config.speciesOrdering = engineEditor.speciesOrdering

  if config.gui.labelRandomise then
    config.gui.labelRandomise.value = config.labelRandomise
  end

  config.gui.btnSpeciesUp.position = {
    engineEditor.paneLayout.labelSpeciesRadio.position[1] + 19,
    engineEditor.paneLayout.labelSpeciesRadio.position[2] - 1
  }
  config.gui.btnSpeciesDown.position = {
    engineEditor.paneLayout.labelSpeciesRadio.position[1] - 27,
    engineEditor.paneLayout.labelSpeciesRadio.position[2] - 1
  }

  config.gui.gender.buttons[1].data = "male"
  config.gui.gender.buttons[2].data = "female"

  config.gui.mode.buttons[1].data.mode = "casual"
  config.gui.mode.buttons[2].data.mode = "survival"
  config.gui.mode.buttons[3].data.mode = "hardcore"

--config.gui.name.enterKey = nil
--config.gui.name.escapeKey = nil
  config.gui.name.focus = false

  config.gui.cancel.callback = "close"
  config.gui.btnToggleClothing = nil

--  config.gui.background.fileHeader = oldHeader --debug grabby part
  return config
end