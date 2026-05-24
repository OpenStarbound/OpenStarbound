function patch(original)
  local image = original:process("?crop=0;0;1;1?scalenearest=236;177")
  image:copyInto({0, 39}, original)
  image:copyInto({0, 0}, original:process("?crop=0;0;236;15")) -- bottom dark area (space for Back/Apply buttons)
  local checkboxArea = original:process("?crop=0;15;236;26") -- checkbox space
  image:copyInto({0, 26}, checkboxArea) -- original one
  image:copyInto({0, 15}, checkboxArea) -- new row
  image:copyInto({0, 37}, original:process("?crop=0;26;236;96")) -- rest of checkboxes up to zoom level
  local checkbox = image:process("?crop=19;37;117;46")
  image:copyInto({119, 37}, checkbox) -- Anti-Aliasing
  image:copyInto({19, 26}, checkbox)  -- New Lighting
  image:copyInto({119, 26}, checkbox) -- Hardware Cursor
  image:copyInto({19, 15}, checkbox) -- HDR
  image:copyInto({119, 15}, checkbox) -- Vsync

  image:copyInto({119, 79}, image:process("?crop=19;79;117;98")) -- Camera Pan Speed

  return image
end
