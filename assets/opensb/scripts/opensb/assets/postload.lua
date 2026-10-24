-- Add object patches
local objects = assets.byExtension("object")
local path = "/objects/opensb/object.patch.lua"
for i = 1, #objects do
  assets.patch(objects[i], path)
end

assets.patch(
  "/interface/windowconfig/songbook.config",
  "/interface/windowconfig/songbook_search_patch.lua"
)

assets.patch(
  "/interface/windowconfig/playerinventory.config",
  "/interface/windowconfig/inventory_cosmetics_patch.lua"
)

-- Relocate songs to the /songs/ folder

local songs = assets.byExtension("abc")
local count = 0

for i = 1, #songs do
  local song = songs[i]
  if song:sub(1, 7) ~= "/songs/" then
    local data = assets.bytes(song)
    assets.erase(song)
    assets.add("/songs" .. song, data)
    count = count + 1
  end
end

if count > 0 then
  sb.logInfo("Moved %s misplaced songs to /songs/", count)
end

-- pre generating the 20 occupant slot animations so we're not wasting time generating the same thing multiple times later
local chestLegsSortOrder = { 14, 8, 2, 0, 15, 9, 3, 1, 4, 5, 6, 7, 10, 11, 12, 13, 16, 17, 18, 19 }
for _, path in ipairs(assets.scan("/humanoid/", "cosmetic.animation")) do
    local merged = {}
    for i = 1, 20 do
        merged = sb.jsonMerge(merged, sb.parseJson(assets.bytes(path):gsub(
            '<slot>', tostring(i)
        ):gsub(
            '"zLevelSlot"%s*:%s*(%d+%.%d+)', '"zLevel":%1' .. string.format('%04i', i)
        ):gsub(
            '"flippedZLevelSlot"%s*:%s*(%d+%.%d+)', '"flippedZLevel":%1' .. string.format('%04i', i)
        ):gsub(
            '"zLevelChestLegsSlot"%s*:%s*(%d+%.%d+)', '"zLevel":%1' .. string.format('%04i', chestLegsSortOrder[i])
        ):gsub(
            '"flippedZLevelChestLegsSlot"%s*:%s*(%d+%.%d+)', '"flippedZLevel":%1' .. string.format('%04i', chestLegsSortOrder[i])
        ):gsub(
            '"zLevelSlot"%s*:%s*(%-%d+%.%d+)', '"zLevel":%1' .. string.format('%04i', 20 - i)
        ):gsub(
            '"flippedZLevelSlot"%s*:%s*(%-%d+%.%d+)', '"flippedZLevel":%1' .. string.format('%04i', 20 - i)
        ):gsub(
            '"zLevelChestLegsSlot"%s*:%s*(%-%d+%.%d+)', '"zLevel":%1' .. string.format('%04i', 20 - chestLegsSortOrder[i])
        ):gsub(
            '"flippedZLevelChestLegsSlot"%s*:%s*(%-%d+%.%d+)', '"flippedZLevel":%1' .. string.format('%04i', 20 - chestLegsSortOrder[i])
        )))
    end
    assets.add(path, sb.printJson(merged))
end
