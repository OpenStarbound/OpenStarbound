# lighting.config

Additional keys in the `lighting` and `intensity` section of `/lighting.config`:

- `pointMaxValue` (float, default `1.0`): the brightest a point light can be. Brighter lights are darkened to this value, keeping their color. Values above `1` allow overbright lights that reach farther than normal.
- `spreadMaxValue` (float, default `1.0`): the same for spread lights.

The lighting border around the screen is sized by `max(spreadMaxAir * spreadMaxValue, pointMaxAir * pointMaxValue)`, so raising either cap enlarges the area that is calculated.
A cap of `N` calculates about `N` times the usual offscreen space, which is costly, and raising `spreadMaxValue` costs more than raising `pointMaxValue`.
Keep these low!

Colors are sent over the network as floats, and this change no longer clamps them when they are read.
Clients and servers with it see overbright lights from each other intact.
Vanilla clients and servers, and builds without this change, clamp colors to `1` when they read them, so they see these lights at full brightness.

A light's beam, i.e., one with `pointBeam` in a light source, is an attenuation amount that does not scale with the light's brightness.
A beam that makes a narrow cone at brightness `1` is much wider at brightness `10`, because the light has more brightness left to lose. Scale `pointBeam` with the brightness to keep the same cone: about `3` gives a 50 degree half-angle at brightness `1`, and about `100` gives a 27 degree half-angle at brightness `10`.
