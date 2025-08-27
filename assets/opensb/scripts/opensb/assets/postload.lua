-- Revert cursor frames if a mod replaced cursors.png with a SD version again
-- Otherwise, scale down our HD cursors
if assets.image("/cursors/cursors.png"):size()[1] == 64 then
  local path = "/cursors/opensb/revert.cursor.patch"
  assets.add(path, '{"scale":null}')
  assets.patch("/cursors/inspect.cursor", path)
  assets.patch("/cursors/link.cursor", path)
  assets.patch("/cursors/pointer.cursor", path)
  path = "/cursors/opensb/revert.frames.patch"
  assets.add(path, '{"frameGrid":{"size":[16,16]}}')
  assets.patch("/cursors/cursors.frames", path)
else
  local cursors = assets.json("/cursors/cursors.frames:frameGrid.names")
  local path = "/cursors/%s.cursor.patch"
  for i = 1, #cursors do
    for j = 1, #cursors[i] do
      assets.add(string.format(path, cursors[i][j]), '{"scale":1}')
    end
  end
end

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
  for i = 1, 20 do
    assets.add(path .. "." .. i, assets.bytes(path):gsub(
      "%<slot%>", tostring(i)
    ):gsub(
      "%.1234", string.format(".%04d", i)
    ):gsub(
      "%.4567", string.format(".%04d", chestLegsSortOrder[i])
    ):gsub(
      "%.4321", string.format(".%04d", 20-i)
    ):gsub(
      "%.7654", string.format(".%04d", 20-chestLegsSortOrder[i])
    ))
  end
end
