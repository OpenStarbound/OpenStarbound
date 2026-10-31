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

-- The sounds of the inventory pane were renamed (someup and somedown became addToStack and takeFromStack).
-- Carry over what a mod set under the old names, so it keeps working.
do
  local config = "/interface/windowconfig/playerinventory.config"
  local sounds = assets.json(config).sounds or {}
  local renamed = {
    { old = "someup", new = "addToStack", file = "/sfx/interface/inventory_someup.ogg" },
    { old = "somedown", new = "takeFromStack", file = "/sfx/interface/inventory_somedown.ogg" }
  }
  local ops = {}
  for _, r in ipairs(renamed) do
    -- the old key counts when its sounds exist (a config copied from before the rename lists files that are gone)
    local list = {}
    for _, sound in ipairs(sounds[r.old] or {}) do
      if assets.exists(sound) then list[#list + 1] = sound end
    end
    -- otherwise a mod may have replaced the old sound file and left the config alone
    if #list == 0 and assets.exists(r.file) then list[1] = r.file end
    if #list > 0 then
      ops[#ops + 1] = { op = "add", path = "/sounds/" .. r.new, value = list }
    end
    if sounds[r.old] ~= nil then
      ops[#ops + 1] = { op = "remove", path = "/sounds/" .. r.old }
    end
  end
  if #ops > 0 then
    local path = "/interface/windowconfig/opensb/sounds_rename.patch"
    assets.add(path, sb.printJson(ops))
    assets.patch(config, path)
  end
end

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
