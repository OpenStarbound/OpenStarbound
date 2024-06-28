--local function drop(color)
--  if type(color) == "table" then
--    for i = 1, #color do
--      color[i] = color[i] * 0.8
--    end
--  end
--end

function patch(object, path)
  if object.pointLight ~= true and (object.lightColor or object.lightColors) then
    object.lightType = "PointAsSpread"
    return object
  --elseif type(object.lightColor) == "table" then
  --  drop(object.lightColor)
  --  return object
  --elseif type(object.lightColors) == "table" then
  --  for i, v in pairs(object.lightColors) do
  --    drop(v)
  --  end
  --  return object
  end
end