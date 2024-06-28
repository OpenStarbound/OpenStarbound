function patch(original)
  local image = original:process("?crop=0;0;1;1?scalenearest=236;166")
  image:copyInto({0, 28}, original)
  image:copyInto({0, 0}, original:process("?crop=0;0;236;96"))
  local checkbox = image:process("?crop=19;26;117;35")
  image:copyInto({119, 26}, checkbox) -- Anti-Aliasing
  image:copyInto({19, 15}, checkbox)  -- New Lighting
  image:copyInto({119, 15}, checkbox) -- Hardware Cursor

  image:copyInto({119, 68}, image:process("?crop=19;68;117;87")) -- Camera Pan Speed

  return image
end