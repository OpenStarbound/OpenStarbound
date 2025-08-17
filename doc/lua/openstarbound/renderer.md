# Renderer

The new *renderer* table is accessible from almost every clientside script and allows configuring shaders.

---

#### `void` renderer.setPostProcessGroupEnabled(String group, bool enabled, [bool save])

Enables or disables a post process shader group. If save is true, this change is saved to configuration as well.

---

#### `bool` renderer.postProcessGroupEnabled(String group)

Returns true if the specified post process group is enabled.

---

#### `Json` renderer.postProcessGroups()

Returns every post process group. Identical to grabbing them from client.config with root.assetJson.

---

#### `Json` renderer.setEffectParameter(String effectName, String parameterName, RenderEffectParameter value)

Sets the specified scriptable parameter of the specified shader effect to the provided value. 
This is accessed from the shader as a uniform and must be defined in the effect's configuration.

---

#### `RenderEffectParameter` renderer.getEffectParameter(String effectName, String parameterName)

Returns the specified scriptable parameter of the specified shader effect.
