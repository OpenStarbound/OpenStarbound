local function modLight(light)
  for i = 1, #light do
    light[i] = light[i] * 0.4
  end
end

function patch(object, path)
  if object.lightColor then
    modLight(object.lightColor)
    object.pointLight = true
    return object;
  elseif object.lightColors then
    for i, v in pairs(object.lightColors) do
      modLight(v)
    end
    object.pointLight = true
    return object;
  end
end