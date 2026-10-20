-- Adds an OpenStarbound page to the end of the credits of the final cinematic.
-- The names come from /opensb/credits/contributors.json, so only that file changes when the list does.
-- The page copies how the vanilla backers are laid out: the heading scrolls up like "BACKERS", then blocks of names, each one
-- starting a little after the one before and every block scrolls up the screen for 12 seconds.

local SOURCE = "/opensb/credits/contributors.json"

local START = 207.9        -- the vanilla credits start their last block of names at 204.4 and the blocks are 3.53 seconds apart
local HEADING_LEAD = 1.3   -- the heading comes this long before the first names, like "BACKERS" before the vanilla names
local BLOCK_SPACING = 3.53
local SCROLL_TIME = 12
local NAMES_PER_LINE = 4
local LINES_PER_BLOCK = 20
local SEPARATOR = "  ^#4d4d4d;|^white;  "

local function clean(name)
  -- ^ starts a text command, so a name can't carry one
  return (tostring(name):gsub("[%^;\"\\%c]", ""))
end

local function scrolling(startTime)
  return {
    startTime = startTime,
    endTime = startTime + SCROLL_TIME
  }
end

local function heading(text, startTime)
  local panel = scrolling(startTime)
  panel.fontColor = jarray{78, 78, 78, 255}
  panel.fontSize = 16
  panel.text = text
  panel.textPosition = {
    position = jarray{650, 380},
    vAnchor = "top",
    horizontalAnchor = "mid",
    desiredWidth = 500
  }
  panel.keyframes = jarray{
    {timecode = 0.0, alpha = 1.0, zoom = 1.5, position = jarray{0, -500}},
    {timecode = SCROLL_TIME, position = jarray{0, 800}}
  }
  return panel
end

local function block(text, startTime)
  local panel = scrolling(startTime)
  panel.text = text
  panel.textPosition = {
    position = jarray{650, 500},
    vAnchor = "top",
    horizontalAnchor = "mid",
    desiredWidth = 500
  }
  panel.keyframes = jarray{
    {timecode = 0.0, alpha = 1.0, zoom = 1.0, position = jarray{0, -500}, textPercentage = 1},
    {timecode = SCROLL_TIME, textPercentage = 1, position = jarray{0, 800}}
  }
  return panel
end

function patch(data, path)
  if not assets.exists(SOURCE) then return data end

  local credits = assets.json(SOURCE)
  local names = {}
  for _, name in ipairs(credits.contributors or {}) do
    local shown = clean(name)
    if shown ~= "" then names[#names + 1] = shown end
  end
  if #names == 0 then return data end

  data.panels[#data.panels + 1] = heading(clean(credits.heading or "OPENSTARBOUND"), START)

  local lines, blockStart = {}, START + HEADING_LEAD
  local function flush()
    if #lines == 0 then return end
    data.panels[#data.panels + 1] = block("\n" .. table.concat(lines, "\n") .. "\n      ", blockStart)
    blockStart = blockStart + BLOCK_SPACING
    lines = {}
  end

  for i = 1, #names, NAMES_PER_LINE do
    local line = {}
    for j = i, math.min(i + NAMES_PER_LINE - 1, #names) do line[#line + 1] = names[j] end
    lines[#lines + 1] = table.concat(line, SEPARATOR) .. SEPARATOR
    if #lines == LINES_PER_BLOCK then flush() end
  end
  flush()

  return data
end
