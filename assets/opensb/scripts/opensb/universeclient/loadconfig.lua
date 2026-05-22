-- Meant to manage loading various miscellaneous things from configuration, such as shader parameters.

local module = {}
modules.config_loader = module

function module.init()
	local shaderConfig = root.getConfiguration("postProcessGroups") or {}
	local postProcessGroups = renderer.postProcessGroups()
	local changes = false
	for k,v in next, shaderConfig do
		local group = postProcessGroups[k]
		if group then
			if group.enabledParameter then
				-- this group might need to handle enable/disabling itself, since maybe it patches another shader that can't be disabled
				for _,e in next, group.enabledParameterEffects do
					renderer.setEffectParameter(e,group.enabledParameter,not not v.enabled)
				end
			end
			if v.parameters then
				for k2,v2 in next, group.parameters do
					local param = v.parameters[k2]
					if param ~= nil then
						if v2.isPasses then
							-- this parameter controls passes
							for _,l in next, v2.layers do
								renderer.setPostProcessLayerPasses(l,param)
							end
						else
							for _,e in next, v2.effects do
								renderer.setEffectParameter(e,k2,param)
							end
						end
					end
				end
			end
		end
	end
end
