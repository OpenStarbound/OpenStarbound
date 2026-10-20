-- Adds an OpenStarbound page to the end of the credits of the final cinematic.
-- The names come from /opensb/credits/contributors.json, so only that file changes when the list does.
-- The page is laid out like the Starbound team pages of the vanilla credits: a heading that stays up, and pages of three columns of three
-- names that are typed out one column after the other, stay for a moment and fade out together. It comes after the vanilla credits have
-- faded to black (the black panel of the vanilla cinematic is fully opaque from 223.5 seconds).

local SOURCE = "/opensb/credits/contributors.json"

local BLACK_START = 223.5     -- when the vanilla credits are fully black
local HEADING_START = 224.5
local HEADING_LEAD = 1.5      -- the first names come this long after the heading, like in the vanilla pages
local PAGE_TIME = 8           -- how long a page of names is up
local PAGE_GAP = 0.2
local COLUMN_DELAY = 1.1      -- each column is typed out this much after the one before it, and they all fade out together
local TYPE_TIME = 1.5
local FADE_TIME = 0.5
local COLUMNS = {500, 650, 800}
local ROWS = 3
local HEADING_Y = 360
local NAMES_Y = 325

local function clean(name)
  -- ^ starts a text command, so a name can't carry one
  return (tostring(name):gsub("[%^;\"\\%c]", ""))
end

local function heading(text, startTime, endTime)
  local duration = endTime - startTime
  return {
    startTime = startTime,
    endTime = endTime,
    fontColor = jarray{78, 78, 78, 255},
    fontSize = 16,
    text = text,
    textPosition = {
      position = jarray{650, HEADING_Y},
      vAnchor = "top",
      horizontalAnchor = "mid",
      desiredWidth = 500
    },
    keyframes = jarray{
      {timecode = 0.0, alpha = 0, zoom = 1.5, textPercentage = 1},
      {timecode = 1.5, alpha = 1.0},
      {timecode = duration - FADE_TIME, alpha = 1.0},
      {timecode = duration, alpha = 0}
    }
  }
end

local function column(names, x, startTime, endTime)
  local duration = endTime - startTime
  return {
    startTime = startTime,
    endTime = endTime,
    text = "\n" .. table.concat(names, "\n") .. "\n      ",
    textPosition = {
      position = jarray{x, NAMES_Y},
      vAnchor = "top",
      horizontalAnchor = "mid",
      desiredWidth = 500
    },
    keyframes = jarray{
      {timecode = 0.0, alpha = 1, zoom = 1, textPercentage = 0},
      {timecode = TYPE_TIME, textPercentage = 1},
      {timecode = duration - FADE_TIME, alpha = 1},
      {timecode = duration, alpha = 0}
    }
  }
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

  local perPage = #COLUMNS * ROWS
  local pageCount = math.ceil(#names / perPage)
  local firstPage = HEADING_START + HEADING_LEAD
  local lastEnd = firstPage + pageCount * (PAGE_TIME + PAGE_GAP) - PAGE_GAP

  local panels = {}
  -- black until the end, like the last panel of the vanilla credits
  panels[#panels + 1] = {
    startTime = BLACK_START,
    endTime = lastEnd + FADE_TIME,
    drawables = jarray{{image = "/cinematics/story/black.png"}},
    keyframes = jarray{{timecode = 0, alpha = 1, zoom = 5, position = jarray{480, 350}, frame = 0}}
  }
  panels[#panels + 1] = heading(clean(credits.heading or "OPENSTARBOUND"), HEADING_START, lastEnd)

  for page = 0, pageCount - 1 do
    local pageStart = firstPage + page * (PAGE_TIME + PAGE_GAP)
    local pageEnd = pageStart + PAGE_TIME
    for c = 1, #COLUMNS do
      local first = page * perPage + (c - 1) * ROWS + 1
      local last = math.min(first + ROWS - 1, #names)
      if first <= last then
        local shown = {}
        for i = first, last do shown[#shown + 1] = names[i] end
        panels[#panels + 1] = column(shown, COLUMNS[c], pageStart + (c - 1) * COLUMN_DELAY, pageEnd)
      end
    end
  end

  for _, panel in ipairs(panels) do data.panels[#data.panels + 1] = panel end
  return data
end
