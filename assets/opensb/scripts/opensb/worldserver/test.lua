local module = {}
modules.test = module

function module.init()
	message.setHandler("type", function()
		return world.type()
	end)
end

function module.update()

end