function patch(data)
  data.sounds.someup = jarray{"/sfx/interface/inventory_someup.ogg"}  
  data.sounds.somedown = jarray{"/sfx/interface/inventory_somedown.ogg"}
  local layout = data.paneLayout

  local function addHiddenIndicator(slot)
    if not slot.children then slot.children = {} end
    slot.children.hidden = {
      type = "image",
      position = {0, 0},
      file = "/interface/inventory/cosmetichidden.png",
      mouseTransparent = true,
      visible = false
    }
    return slot
  end

  local function createCosmeticSlot(origin, offset, i)
    local name = "cosmetic" .. i
    if layout[name] then return end
    layout[name] = addHiddenIndicator{
      type = "itemslot",
      position = {origin.position[1] + offset, origin.position[2]},
      backingImage = origin.backingImage,
      showBackingImageWhenFull = true,
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
  
  addHiddenIndicator(layout.legs)
  addHiddenIndicator(layout.chest)
  addHiddenIndicator(layout.head)
  addHiddenIndicator(layout.back)
  addHiddenIndicator(layout.legsCosmetic)
  addHiddenIndicator(layout.chestCosmetic)
  addHiddenIndicator(layout.headCosmetic)
  addHiddenIndicator(layout.backCosmetic)
  
  layout.portrait.renderHumanoid = true
  return data
end
