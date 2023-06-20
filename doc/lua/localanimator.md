The *localAnimator* table provides bindings used by client side animation scripts (e.g. on objects and active items) to set drawables/lights and perform rendering actions.

---

#### `void` localAnimator.playAudio(`String` sound, [`int` loops], [`float` volume])

Immediately plays the specified sound, optionally with the specified loop count and volume.

---

#### `void` localAnimator.spawnParticle(`Json` particleConfig, `Vec2F` position)

Immediately spawns a particle with the specified name or configuration at the specified position.

---

#### `void` localAnimator.addDrawable(`Drawable` drawable, [`String` renderLayer])

Adds the specified drawable to the animator's list of drawables to be rendered. If a render layer is specified, this drawable will be drawn on that layer instead of the parent entity's render layer. Drawables set in this way are retained between script ticks and must be cleared manually using localAnimator.clearDrawables().

The drawable object must specify exactly one of the following keys to define its type:

* [`pair<Vec2F, Vec2F>` __line__] - Defines this drawable as a line between the specified two points.
* [`List<Vec2F>` __poly__] - Defines the drawable as a polygon composed of the specified points.
* [`String` __image__] - Defines the drawable as an image with the specified asset path.

The following additional keys may be specified for any drawable type:

* [`Vec2F` __position__] - Relative position of the drawable.
* [`Color` __color__] - Color for the drawable. Defaults to white.
* [`bool` __fullbright__] - Specifies whether the drawable is fullbright (ignores world lighting).

The following additional key may be specified for line drawables:

* [`float` __width__] - Specifies the width of the line to be rendered.

The following transformation options may be specified for image drawables. Note that if a __transformation__ is specified, it will be used instead of other specific transformation operations.

* [`Mat3F` __transformation__]
* [`bool` __centered__]
* [`float` __rotation__]
* [`bool` __mirrored__]
* [`float` __scale__]

---

#### `void` localAnimator.clearDrawables()

Clears the list of drawables to be rendered.

---

#### `void` localAnimator.addLightSource(`Json` lightSource)

Adds the specified light source to the animator's list of light sources to be rendered. Light sources set in this way are retained between script ticks and must be cleared manually using localAnimator.clearLightSources(). The configuration object for the light source accepts the following keys:

* `Vec2F` __position__
* `Color` __color__
* [`bool` __pointLight__]
* [`float` __pointBeam__]
* [`float` __beamAngle__]
* [`float` __beamAmbience__]

---

#### `void` localAnimator.clearLightSources()

Clears the list of light sources to be rendered.
