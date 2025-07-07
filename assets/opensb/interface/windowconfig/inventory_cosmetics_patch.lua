function patch(data)
  data.sounds.someup = jarray{"/sfx/interface/inventory_someup.ogg"}  
  data.sounds.somedown = jarray{"/sfx/interface/inventory_somedown.ogg"}
  local layout = data.paneLayout

  local function createCosmeticSlot(origin, offset, i)
    local name = "cosmetic" .. i
    if layout[name] then return end
    layout[name] = {
      type = "itemslot",
      position = {origin.position[1] + offset, origin.position[2]},
      backingImage = origin.backingImage,
      data = {tooltipText = "Cosmetic " .. i},
      zlevel = 5,
      children = {
        back = {
          type = "image",
          position = {-2, 0},
          file = "/interface/inventory/cosmeticback.png",
          mouseTransparent = true
        }
      }
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