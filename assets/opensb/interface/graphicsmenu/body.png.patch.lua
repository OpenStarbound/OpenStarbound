function patch(image)
  -- Camera Pan Speed
  image:copyInto({119, 68}, image:process("?crop=19;68;117;87"))
  -- Anti-Aliasing
  image:copyInto({119, 26}, image:process("?crop=19;26;117;35"))
end