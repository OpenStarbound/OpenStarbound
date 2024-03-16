-- revert cursor frames if a mod replaced cursors.png with a SD version again
if assets.image("/cursors/cursors.png"):size()[1] == 64 then
  local path = "/cursors/opensb/revert.cursor.patch"
  assets.add(path, '{"scale":null}')
  assets.patch("/cursors/inspect.cursor", path)
  assets.patch("/cursors/link.cursor", path)
  assets.patch("/cursors/pointer.cursor", path)
  path = "/cursors/opensb/revert.frames.patch"
  assets.add(path, '{"frameGrid":{"size":[16,16]}}')
  assets.patch("/cursors/cursors.frames", path)
end