local module = {}
modules.messages = module

function module.init()
	message.setHandler("keepAlive", function(_, _, time)
		return world.setExpiryTime(tonumber(time) or 0)
	end)
end

function module.update()

end