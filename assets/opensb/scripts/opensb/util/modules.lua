modules = setmetatable({}, {__call = function(this, path, names)
	for i, name in pairs(names) do
		require(path .. name .. ".lua")
	end
end})

local modules, type = modules, type
local function call(func, ...)
	if type(func) == "function" then
		return func(...)
	end
end

function init(...)
	script.setUpdateDelta(1)
	for i, module in pairs(modules) do
		call(module.init, ...)
	end
end

function update(...)
	for i, module in pairs(modules) do
		call(module.update, ...)
	end
end

function uninit(...)
	for i, module in pairs(modules) do
		call(module.uninit, ...)
	end
end