function patch(object, path)
  if object.pointLight ~= true and (object.lightColor or object.lightColors) then
    object.lightType = "PointAsSpread"
    return object;
  end
end