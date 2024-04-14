function patch(image)
  -- Camera Pan Speed
  image:copyInto({119, 68}, image:process("?crop=19;68;117;87"))
  local checkbox = image:process("?crop=19;26;117;35")
  -- Anti-Aliasing
  image:copyInto({119, 26}, checkbox)
  -- Object Lighting
  image:copyInto({19, 15}, checkbox)
end