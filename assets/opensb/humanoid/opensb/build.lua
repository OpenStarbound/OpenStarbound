
-- identity is the NPC or player's identity
-- humanoidConfig is the merged data of the defined humanoidConfig for that species merged with its humanoidOverrides
-- npcHumanoidConfig is only passed when an NPC has its own humanoidConfig defined in its npcType
-- the returned value is the humanoidConfig that will be effectively used for the humanoidEntity
-- this means you can have things like movement parameters, death particles, or the numerous offsets change based on identity values
-- and if you're using animation configs for the species, you can change things a whole lot more
function build(identity, humanoidConfig, npcHumanoidConfig)
    if npcHumanoidConfig then sb.logInfo("special NPC config!") return npcHumanoidConfig end

    humanoidConfig.animation = {
        animatedParts = {
            parts = {
                test = {
                    properties = {
						centered = true,
						image = "dead.png"
					}
				}
			}
		}
    }
	if identity.species == "human" then
        humanoidConfig.animation.animatedParts.parts.waow = {
            properties = {
				centered = true,
				image = "/opensb/coconut.png"
			}
		}
	end
	return humanoidConfig
end
