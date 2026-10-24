-- Revert cursor frames if a mod replaced cursors.png with a SD version again
-- Otherwise, scale down our HD cursors
if assets.image("/cursors/cursors.png"):size()[1] == 64 then
  local path = "/cursors/hdCursor/revert.cursor.patch"
  assets.add(path, '{"scale":null}')
  assets.patch("/cursors/inspect.cursor", path)
  assets.patch("/cursors/link.cursor", path)
  assets.patch("/cursors/pointer.cursor", path)
  path = "/cursors/hdCursor/revert.frames.patch"
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
