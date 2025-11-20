--local function drop(color)
--  if type(color) == "table" then
--    for i = 1, #color do
--      color[i] = color[i] * 0.8
--    end
--  end
--end
function hash(str)
  h = 5381;

  for c in str:gmatch "." do
    h = math.fmod(((h << 5) + h) + string.byte(c), 2147483648)
  end
  return h
end
function hsv2rgb(h, s, v)
    local C = v * s
    local m = v - C
    local r, g, b = m, m, m
    if h == h then
        local h_ = (h % 1.0) * 6
        local X = C * (1 - math.abs(h_ % 2 - 1))
        C, X = C + m, X + m
        if h_ < 1 then
            r, g, b = C, X, m
        elseif h_ < 2 then
            r, g, b = X, C, m
        elseif h_ < 3 then
            r, g, b = m, C, X
        elseif h_ < 4 then
            r, g, b = m, X, C
        elseif h_ < 5 then
            r, g, b = X, m, C
        else
            r, g, b = C, m, X
        end
    end
    return math.floor(r * 255), math.floor(g * 255), math.floor(b *255)
end

function patch(object, path)
  if object.pointLight ~= true and (object.lightColor or object.lightColors) then
    object.lightType = "PointAsSpread"
    --elseif type(object.lightColor) == "table" then
    --  drop(object.lightColor)
    --  return object
    --elseif type(object.lightColors) == "table" then
    --  for i, v in pairs(object.lightColors) do
    --    drop(v)
    --  end
    --  return object
  end
  math.randomseed(hash(object.objectName))
  if not object.inputNodesConfig then
    object.inputNodesConfig = jarray()
    local r, g, b = hsv2rgb(math.random(), 1, 1)
    for i, _ in ipairs(object.inputNodes or {}) do
      object.inputNodesConfig[i] = {
        color = { r, g, b }
      }
    end
  end
  if not object.outputNodesConfig then
    object.outputNodesConfig = jarray()
    local r, g, b = hsv2rgb(math.random(), 1, 1)
    for i, _ in ipairs(object.outputNodes or {}) do
      object.outputNodesConfig[i] = {
        color = { r, g, b }
      }
    end
  end

  return object
end
