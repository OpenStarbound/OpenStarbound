-- Meant to manage loading various miscellaneous things from configuration, such as shader parameters.

local module = {}
modules.config_loader = module

function module.init()
	local shaderConfig = root.getConfiguration("postProcessGroups") or {}
	local postProcessGroups = renderer.postProcessGroups()
	local changes = false
	for k,v in next, shaderConfig do
		local group = postProcessGroups[k]
		if group and v.parameters then
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
