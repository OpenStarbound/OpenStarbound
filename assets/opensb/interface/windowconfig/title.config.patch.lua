function patch(data)
  for i, v in pairs(data.mainMenuButtons) do
    if not v.rightAnchored then
      v.offset[2] = v.offset[2] + (v.key == "quit" and -5 or 15)
    end
  end
  data.skyBackdropDarken = jarray{0, 0, 0, 64}
  local rng = sb.makeRandomSource()
  local num = rng:randUInt(300)
  local terry = num == rng:randUInt(10) -- thanks to Discord user arcanearcaneflower for letting me use this sprite!
  local logo = terry and "starraria.png" or (rand == 0 and "barst" or "starb") .. "ound.png"
  data.backdropImages = jarray{
    jarray{
      jarray{0, 0}, 
      "/interface/title/" .. logo,
      terry and 0.75 or 0.5,
      jarray{0.5, terry and 0.75 or 0.5},
      { terry = terry }
    }
  }
  data.scripts = jarray{"/interface/title/title.lua"}
  return data
end
