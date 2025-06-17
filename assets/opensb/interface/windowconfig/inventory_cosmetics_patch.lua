function patch(data)
  data.sounds.someup = jarray{"/sfx/interface/inventory_someup.ogg"}  
  data.sounds.somedown = jarray{"/sfx/interface/inventory_somedown.ogg"}
  local layout = data.paneLayout
  layout.imgCosmeticBack = {
    type = "image",
    position = {layout.legsCosmetic.position[1] + 18, layout.legsCosmetic.position[2]},
    file = "/interface/inventory/cosmeticsback.png",
    zlevel = 3,
    visible = false
  }

  local function createCosmeticSlot(origin, offset, i)
    layout["cosmetic" .. i] = {
      type = "itemslot",
      position = {origin.position[1] + offset, origin.position[2]},
      backingImage = origin.backingImage,
      data = {tooltipText = "Cosmetic " .. i},
      zlevel = 4
    }
  end

  for i = 1, 4 do
    local offset = 20 * i
    createCosmeticSlot(layout.legsCosmetic, offset, i)
    createCosmeticSlot(layout.chestCosmetic, offset, i + 4)
    createCosmeticSlot(layout.headCosmetic, offset, i + 8)
  end

  layout.portrait.renderHumanoid = true
  return data
end