require "/scripts/vec2.lua"

local canvas
local backdropImages
function init()
  canvas = widget.bindCanvas("canvas")
  backdropImages = root.assetJson("/interface/windowconfig/title.config:backdropImages")
end

local logoRotation = 0
local logoRotationDirection = -1
local logoRotationSpeed = 1
local logoScale = 1
local logoScaleDirection = 1
local logoScaleSpeed = 1
local terry = false

function update(dt)
  if not terry then return end
  -- yes, they really did it like this
  if logoRotation > 0.09 then
    logoRotation = logoRotation + logoRotationSpeed * dt
    if logoRotationSpeed > 0 then logoRotationSpeed = 0 end
  end

  logoRotation = logoRotation + logoRotationSpeed * 4e-06
  logoRotationDirection = (logoRotation > 0.08 and -1) or (logoRotation < -0.08 and 1) or logoRotationDirection

  if logoRotationDirection == 1 and logoRotationSpeed < 20 then
    logoRotationSpeed = logoRotationSpeed + dt * 60
  elseif logoRotationDirection == -1 and logoRotationSpeed > -20 then
    logoRotationSpeed = logoRotationSpeed - dt * 60
  end

  logoScale = logoScale + logoScaleSpeed * 9e-06
  logoScaleDirection = (logoScale > 1.35 and -1) or (logoScale < 1 and 1) or logoScaleDirection

  if logoScaleDirection == 1 and logoScaleSpeed < 50 then
    logoScaleSpeed = logoScaleSpeed + dt * 60
  elseif logoScaleDirection == -1 and logoScaleSpeed > -50 then
    logoScaleSpeed = logoScaleSpeed - dt * 60
  end
end

function render(data)
  canvas:clear()
  local window = canvas:size()
  for i, v in pairs(backdropImages) do
    local offset, image, scale, origin, misc = table.unpack(v)
    origin = origin or {0.5, 1.0}
    local imageSize = vec2.mul(root.imageSize(image), scale)
    local position = vec2.add(vec2.mul(window, origin), vec2.sub(offset, vec2.mul(imageSize, vec2.sub(origin, 0.5))))
    if misc.terry then
      terry = true
      canvas:drawImageDrawable(image, position, scale * logoScale, {255, 255, 255}, logoRotation)
    else
      canvas:drawImageDrawable(image, position, scale)
    end 
  end
end