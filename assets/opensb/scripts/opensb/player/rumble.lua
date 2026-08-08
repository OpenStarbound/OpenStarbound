local module = {}
modules.rumble = module

local lastHealth = nil
local lastDamageTick = nil
local wasTeleporting = false
local vehicleState = {
  lastOnGround = false,
  lastYVelocity = 0
}

function module.init()
  sb.logInfo("[rumble] init called, input=%s, input.rumble=%s", tostring(input), tostring(input and input.rumble))
  lastHealth = status.resource("health")
  lastDamageTick = 0
  wasTeleporting = false
  vehicleState.lastOnGround = false
  vehicleState.lastYVelocity = 0
end

function module.update(dt)
  -- debug: log once
  if not module._logged then
    sb.logInfo("[rumble] update called, input=%s, rumble=%s", tostring(input), tostring(input and input.rumble))
    module._logged = true
  end
  if not input or not input.rumble then return end

  updateDamageRumble()
  updateTeleportRumble()
  updateVehicleLandingRumble()
end

-- Rumble when player takes damage, scaled by damage fraction
function updateDamageRumble()
  local currentHealth = status.resource("health")
  local maxHealth = status.resourceMax("health")

  if lastHealth and currentHealth < lastHealth and maxHealth > 0 then
    local damageFraction = (lastHealth - currentHealth) / maxHealth
    local intensity = math.max(0.2, math.min(1.0, damageFraction * 4.0))
    local duration = math.floor(150 + damageFraction * 300)
    sb.logInfo("[rumble] DAMAGE: fraction=%.3f intensity=%.3f duration=%d", damageFraction, intensity, duration)
    input.rumble(intensity, intensity * 0.5, duration)
  end

  lastHealth = currentHealth
end

-- Rumble on teleport beam start
function updateTeleportRumble()
  local teleporting = player.isTeleporting() or player.isTeleportingOut()

  if teleporting and not wasTeleporting then
    input.rumble(0.3, 0.6, 500)
  end

  wasTeleporting = teleporting
end

-- Rumble when a vehicle lands after falling
function updateVehicleLandingRumble()
  local loungeData = player.loungingIn()
  if not loungeData then
    vehicleState.lastOnGround = false
    vehicleState.lastYVelocity = 0
    return
  end

  local vehicleId = loungeData
  local onGround = world.entityExists(vehicleId) and world.entity(vehicleId):onGround()
  local velocity = world.entityVelocity(vehicleId)
  local yVelocity = velocity and velocity[2] or 0

  -- Detect landing: was airborne, now grounded, was falling fast
  if onGround and not vehicleState.lastOnGround and vehicleState.lastYVelocity < -10 then
    -- Check not in zero-G (no rumble for docking in space)
    local inZeroG = world.entity(vehicleId):zeroG()
    if not inZeroG then
      local impactSpeed = -vehicleState.lastYVelocity
      local intensity = math.max(0.3, math.min(1.0, impactSpeed / 40.0))
      local duration = math.floor(200 + impactSpeed * 5)
      input.rumble(intensity, intensity * 0.7, duration)
    end
  end

  vehicleState.lastYVelocity = yVelocity
  vehicleState.lastOnGround = onGround or false
end
