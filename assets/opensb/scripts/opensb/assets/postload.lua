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