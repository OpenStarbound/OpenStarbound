-- these are just the names of the variables internally and how they're used on the creation screen, do whatever you want with them here!
-- but probably put name species and gender as the ones supplied, thats probably a good idea
-- I've supplied the bare minimum as a template here, do with it what you will
function create(name, species, genderIndex, bodyColor, alty, hairChoice, heady, shirtChoice, shirtColor, pantsChoice, pantsColor, personality, ...)
    -- these values are zero indexed!

    local speciesConfig = root.speciesConfig(species)
    local humanoidConfig = sb.jsonMerge(root.assetJson(speciesConfig.humanoidConfig or "/humanoid.config"), speciesConfig.humanoidOverrides or {})

    genderIndex = math.fmod(genderIndex, #speciesConfig.genders)
    personality = math.fmod(personality, #humanoidConfig.personalities)

    local personalityIdle, personalityArmIdle, personalityHeadOffset, personalityArmOffset = table.unpack(humanoidConfig.personalities[personality+1])

    local identity = {
        name = name,
        species = species,
        gender = speciesConfig.genders[genderIndex+1].name,
        hairGroup = "",
        hairType = "",
        hairDirectives = "",
        bodyDirectives = "",
        emoteDirectives = "",
        facialHairGroup = "",
        facialHairType = "",
        facialHairDirectives = "",
        facialMaskGroup = "",
        facialMaskType = "",
        facialMaskDirectives = "",
        personalityIdle = personalityIdle,
        personalityArmIdle = personalityArmIdle,
        personalityHeadOffset = personalityHeadOffset,
        personalityArmOffset = personalityArmOffset,
        color = {51, 117, 237, 255},
    }
    local parameters = {
        choices = { genderIndex, bodyColor, alty, hairChoice, heady, shirtChoice, shirtColor, pantsChoice, pantsColor, personality, ... },
        --this you can do a lot with, see the humanoid build script
    }
    local armor = {
        head = nil,
        chest = nil,
        legs = nil,
        back = nil,
        headCosmetic = nil,
        chestCosmetic = nil,
        legsCosmetic = nil,
        backCosmetic = nil,
        cosmetic1 = nil,
        cosmetic2 = nil,
        cosmetic3 = nil,
        cosmetic4 = nil,
        cosmetic5 = nil,
        cosmetic6 = nil,
        cosmetic7 = nil,
        cosmetic8 = nil,
        cosmetic9 = nil,
        cosmetic10 = nil,
        cosmetic11 = nil,
        cosmetic12 = nil,
    }
    return identity, parameters, armor
end
