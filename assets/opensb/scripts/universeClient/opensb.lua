submodules = {}

require "/scripts/universeClient/opensb/voice_manager.lua"

local submodules, type = submodules, type
local function call(func, ...)
	if type(func) == "function" then
		return func(...)
	end
end

function init(...)
	script.setUpdateDelta(1)
	for i, module in pairs(submodules) do
		call(module.init, ...)
	end
end

function update(...)
	for i, module in pairs(submodules) do
		call(module.update, ...)
	end
end

function uninit(...)
	for i, module in pairs(submodules) do
		call(module.uninit, ...)
	end
end