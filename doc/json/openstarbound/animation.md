# animation

These are json files that typically have the `.animation` extension in starbound's assets, however this is not required.

All changes that illicit new behavior in how the animator behaves are locked behind defining a `version` in the root of the config to enable them.

Retail is version 0

Current OSB animator is version 1

## includes

In the root of the config, one can define an array `includes` which will contain the paths to other animation configs that will be merged underneath the current animation config when the animator is initialized.

This is intended to allow `animationCustom` parameters on weapons or objects to include large modifications to the animation, without balooning out the parameter data that is stored in the item, but is also just useful for modular animations to not need to be copied.

## animation transform properties

The `properties` or `frameProperties` of a stateType's state can define transformations that are added onto a transform group's transformation.

This is intended to be useful for transformations that are tied to an animation state, and should always be applied in the same way for the same frame every time, as using the networked transformation groups in script often leads to desync on clients.

If the transformation group is marked as interpolated, then it will interpolate the resulting transformation between the current frame and the next.

Transformation properties are defined as the name of the transformation group, and then an array of transformations to apply.

Here is an example used in `frameProperties`
```
"transformGroupName" : [
    [["reset"],	["scale",[1, 1]],	["rotateDegrees", 0],	["translate",[0, 0]]],
    [["reset"],	["scale",[0, 0]],	["rotateDegrees", 180],	["translate",[0, -2]]]
]
```
If the transform group named `transformGroupName` is interpolated, then between frame 1 and frame 2, the transform group would be scaled down to 0, rotated upside down, and lowered two blocks. Do note the `reset` at the beginning of the array, if it were not there, the transformation would continually build on itself each frame.

Here is another example that could be within `properties`
```
"transformGroupName" : [["rotateDegrees", 1]]
```
With the lack of a `reset` this transformation would be applied for every update tick, which would make `transformGroupName` continually rotate by 1 degree every tick endlessly.

The transformations of only the highest priority animation for the relevant transformGroup are applied.

The animation transform property is an array of actions, which are themselves an array, where the first index is a string for the action to perform, and the following indices are the arguments for that action

These are the available actions
- `["reset"]` resets the animation transform.
- `["translate", Vec2F translation]` translates the animation transform
- `["rotate", float rotation, Maybe<Vec2F> rotationCenter]`, rotates the animation transform in radians by the optional rotation center, falls back to the `rotationCenter` property of the state if it exists.
- `["rotateDegrees", float rotation, Maybe<Vec2F> rotationCenter]`, rotates the animation transform in degrees by the optional rotation center, falls back to the `rotationCenter` property of the state if it exists.
- `["scale", Vec2F scale, Maybe<Vec2F> scaleCenter]`, scales the animation transform by the optional scale center, falls back to the `scalingCenter` property of the state if it exists.
- `["transform", float a, float b, float c, float d, float tx, float ty]`, transforms the animation transform.

These are also available in part state `properties` and `frameProperties` by defining a `transforms` property, it can also be interpolated by setting the `interpolated` property on the part to true.

## animation tag properties
The `properties` or `frameProperties` of a stateType's state can define an `animationTags` JsonObject, where each key : value pair is a tag that gets set while it is active.

## flipped image property
`properties` or `frameProperties` can contain a `flippedImage` property that gets used for the image when the animator is flipped, much like how there is a `flippedZlevel` property
