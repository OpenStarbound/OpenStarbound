# Controller Support

This document covers the controller support implementation for OpenStarbound: what was built, the architectural decisions behind it, and the non-obvious pitfalls discovered during development. Written during the initial implementation on a Steam Deck with Frackin Universe installed.

## Overview

Controller support adds full gamepad input to OpenStarbound with analog stick movement, twin-stick aiming, configurable input modes, haptic feedback, and a settings UI. It's designed to be upstream-compatible — the legacy keyboard binding system is untouched, and controller binds are additive through the existing OpenStarbound bind system.

## Architecture

### Input Mode System

Four input modes, selectable in Options → Controller:

| Mode | Aim Source | Cursor Source | Use Case |
|------|-----------|---------------|----------|
| **Off** | Mouse only | Mouse only | Disable controller entirely |
| **Auto** | Last device wins | Last device wins | General PC gaming with controller nearby |
| **Gamepad** | Right stick | Virtual cursor (L3 toggle) | Pure controller, no mouse |
| **Hybrid** | Right stick | Mouse/trackpad | Steam Deck, DualSense with trackpad |

**Why Hybrid exists:** Steam Deck and DualSense have trackpads that function as native mouse input. Hybrid mode decouples aim (right stick) from UI cursor (trackpad), letting users do both simultaneously — e.g., mining a wall with the right stick while rearranging inventory with the trackpad. This is not possible in Auto or Gamepad modes.

**Why Off exists:** Some players use controllers for other applications while playing with keyboard/mouse. Without Off mode, accidental controller input (bumped stick, drift) would interfere with gameplay.

### Binding System

The implementation uses **two parallel binding systems** that coexist without conflict:

1. **Legacy `KeyBindings` system** — Maps `KeyChord` (key + modifiers) → `InterfaceAction`. This is what vanilla Starbound and mods like Frackin Universe use. **Untouched.**

2. **OpenStarbound `Input` bind system** — Maps `InputVariant` (Key, MouseButton, or ControllerButton) → named binds via `.binds` asset files. Already existed for OpenStarbound features (zoom, voice, building).

Controller support adds `controller.binds` with entries whose IDs match `InterfaceAction` names (e.g., `PlayerJump`, `InterfaceInventory`). The `isActionTaken()` / `isActionTakenEdge()` functions in `StarClientApplication.cpp` were extended to check **both** systems:

```cpp
bool ClientApplication::isActionTaken(InterfaceAction action) const {
  // Check legacy keyboard bindings
  for (auto keyEvent : m_heldKeyEvents) {
    if (m_guiContext->actions(keyEvent).contains(action))
      return true;
  }
  // Also check controller binds via the new Input bind system
  if (auto name = InterfaceActionNames.maybeRight(action)) {
    if (m_input->bindHeld("controller", *name))
      return true;
  }
  return false;
}
```

This means mods that configure the `bindings` config object continue to work unchanged. Controller binds are purely additive.

### Trigger Axis-to-Button Conversion

SDL3 reports LT/RT as analog axes (`ControllerAxis::TriggerLeft/Right`), not buttons. The bind system only handles `ControllerButton`. We added `TriggerLeft` and `TriggerRight` to the `ControllerButton` enum and generate synthetic button press/release events in `processInput` when trigger axes cross a 0.5 threshold.

### Analog Stick Processing

**Left stick (movement):** Always active regardless of input mode (except Off). Uses deadzone normalization — values below the deadzone are ignored, values above are remapped to 0.0–1.0 range. This feeds into `Player::setMoveVector()` which provides proportional walk/run speed.

**Right stick (aim):** Computes a world-space offset from the player (or vehicle) position. The offset's direction and distance are determined by stick angle and magnitude. When the stick returns to center, the direction is retained with a small offset (0.5 tiles for player, 4.0 tiles for vehicles) to maintain consistent character facing.

**Why relative offset, not fixed world position:** Early implementation stored a fixed world-space aim position. Walking past it caused the angle from player to aim point to change, flipping the character's facing direction. Relative offset means the aim moves with the player — facing stays consistent.

### Virtual Cursor

In Gamepad mode, L3 toggles a virtual cursor driven by the right stick. The cursor:
- Moves with quadratic acceleration (precise at gentle tilt, fast at full)
- Warps the hardware cursor via `SDL_WarpMouseInWindow`
- Sends synthetic `MouseMoveEvent`, `MouseButtonDownEvent/Up` through `MainInterface::handleInputEvent`
- RT = left click, LT = right click
- Auto-deactivates when switching away from Gamepad mode

This means every existing pane, widget, and mod UI works without modification — the virtual cursor is indistinguishable from a real mouse from the pane system's perspective.

### R3 Right-Click in Panes

Several panes require right-click (navigation console for travel, inventory for stack splitting). In Hybrid mode, trackpad click is left-click only. R3 (right stick click) generates a synthetic right-click when any window/modal pane is open. When no pane is open, R3 is camera pan (existing CameraShift behavior). These contexts are mutually exclusive — you don't pan the camera while navigating menus.

### Vehicle/Mech Handling

**Aim origin:** When the player is lounging in a vehicle (mech, hoverbike, etc.), `m_player->position()` returns the anchor point inside the vehicle, which is offset from the visual center. The aim origin switches to the vehicle entity's position via world entity lookup.

**Direction retention offset:** Mechs are ~7 tiles tall with collision polys extending ~3.5 tiles from center. The 0.5-tile direction-retention offset used for on-foot players is inside the mech body, causing direction instability. Vehicles use a 4.0-tile offset instead.

**Initial direction:** The default aim direction (before the stick is used) is set once and marked active immediately. Without this, reading `facingDirection()` every frame creates a feedback loop — facingDirection depends on aim position, which depends on facingDirection — causing rapid visual flickering in mechs.

## Footguns and Non-Obvious Issues

### Steam Deck Dual Controller Conflict

**The single most time-consuming bug.** Steam Deck registers two gamepads via SDL3: the physical Neptune controller AND a Steam Virtual Gamepad. Both send axis events, but our code originally ignored the controller ID — whichever device's event arrived last in the frame won. The two devices' events alternated, causing raw stick values to oscillate between ~0.9 (real stick) and ~0.03 (idle virtual pad) every frame.

**Fix:** Track an `m_activeController` ID. The first controller that sends significant input locks in. All axis events from other controllers are ignored. The active controller resets after 120 frames (2 seconds) of inactivity, allowing controller hot-swapping.

**Symptom:** Left stick movement stutters — character moves for one frame, stops for one frame, moves, stops. D-pad works perfectly (because d-pad fires button events, not axis events, and goes through the `InterfaceAction` path instead of `setMoveVector`).

### Steam Deck Trackpad Mouse Events

The Steam Deck's trackpads generate constant tiny `MouseMoveEvent`s even when untouched. In Auto mode, any mouse event would flip `m_gamepadActive = false`, zeroing movement for a frame. Fixed by requiring mouse movement magnitude > 2.0 pixels to trigger a mode switch.

### Left Stick Movement Must Be Mode-Independent

Early implementation gated left stick movement on `useGamepadMovement` which depended on the input mode state. This was wrong — there's no scenario where pushing a stick should not move the character. The mode system only controls what the **right stick** does. Left stick movement is now unconditional (except in Off mode).

### `processControls()` Priority Conflict

In `StarPlayer.cpp`, `processControls()` checks if `m_pendingMoves` already contains Left/Right before applying `m_moveVector`. If keyboard movement (`moveLeft()`/`moveRight()`) runs in the same frame and adds to `m_pendingMoves`, the analog stick's `setMoveVector` is silently ignored. This caused issues when d-pad binds triggered `moveRight()` through `isActionTaken` while the stick was also active.

### Slider Widget JSON Format

Starbound's slider widget parser expects `range` as a 3-element array `[min, max, delta]`. Using a 2-element array causes `rangeConfig.get(2).toInt()` to read null, crashing with "Improper conversion to int from null" during OptionsMenu construction. The `gridImage` property is also mandatory — omitting it crashes with "No such key in Json::get".

### BaseScriptPane Doesn't Have Root Callbacks

A plain `BaseScriptPane` only gets `pane`, `widget`, and `config` Lua callbacks — not `root`. The controller settings pane needs `root.getConfiguration`/`root.setConfiguration` to read/write game settings. The `ControllerSettingsPane` class extends `BaseScriptPane` and calls `m_script.setLuaRoot(make_shared<LuaRoot>())` to register root callbacks. Note: `LuaRoot` automatically registers the `root` callback table — explicitly adding it via `addCallbacks("root", ...)` causes a "Duplicate callbacks" crash.

### Checkable Buttons Toggle Independently

Starbound's `checkable` button property creates a self-toggling button. When used in a radio-group pattern (only one should be selected), clicking a button toggles its own state independently of others. The button's internal toggle fires after the callback, potentially undoing `setChecked` calls. **Fix:** Use non-checkable buttons and swap their base images via `widget.setButtonImages()` in Lua.

## Files Modified

### C++ Source

| File | Changes |
|------|---------|
| `source/core/StarInputEvent.hpp/cpp` | Added `TriggerLeft`/`TriggerRight` to `ControllerButton` enum |
| `source/client/StarClientApplication.hpp/cpp` | Input mode system, stick processing, aim logic, virtual cursor, rumble, hotbar cycling, beam combo, R3 right-click, all controller action dispatch |
| `source/frontend/StarMainInterface.hpp/cpp` | `setOverrideAim()` to prevent mouse cursor overwriting controller aim |
| `source/frontend/StarOptionsMenu.hpp/cpp` | Controller settings button and pane wiring |
| `source/frontend/StarControllerSettingsPane.hpp/cpp` | New class extending BaseScriptPane with root Lua callbacks |
| `source/frontend/CMakeLists.txt` | Added ControllerSettingsPane files |
| `source/application/StarApplicationController.hpp` | `rumble()`, `rumbleTriggers()`, `activeControllerName()` virtual methods |
| `source/application/StarMainApplication_sdl.cpp` | SDL3 rumble implementation, controller name lookup |
| `source/game/StarPlayer.hpp/cpp` | `isFiring()` public method |
| `source/game/StarVehicle.hpp/cpp` | `onGround()` public method |

### Assets

| File | Purpose |
|------|---------|
| `assets/opensb/binds/controller.binds` | Default controller button mappings for all game actions |
| `assets/opensb/binds/opensb.binds` | Virtual cursor toggle/click binds |
| `assets/opensb/interface/opensb/controller/controller.config` | Controller settings pane layout |
| `assets/opensb/interface/opensb/controller/controller.lua` | Settings pane logic (mode selection, sliders) |
| `assets/opensb/interface/opensb/controller/body.png` | Opaque pane background |
| `assets/opensb/interface/opensb/controller/header.png` | Opaque pane header |
| `assets/opensb/interface/opensb/controller/footer.png` | Opaque pane footer |
| `assets/opensb/interface/optionsmenu/optionsmenu.config.patch` | Adds Controller button to options menu |

## Known Limitations

- **No in-game controller selection UI.** Active controller is auto-detected (first with significant input wins). Configurable via `starbound.config` but no dropdown menu.
- **Title screen is mouse-only.** Character selection and multiplayer setup require mouse/trackpad.
- **Star map navigation requires R3 right-click** (or trackpad in hybrid mode) since travel is triggered by right-click in the vanilla cockpit Lua.
- **No HUD button prompt glyphs.** The game doesn't show controller button icons in-game (e.g., "Press A to interact"). Only the keybinding menu shows controller button names.
- **No DualSense adaptive trigger support.** SDL3 doesn't expose the DualSense HID protocol for trigger resistance. Basic rumble and trigger rumble work.
- **Double-duty binds (L3, R3) aren't rebindable as separate functions.** L3 is virtual cursor toggle in Gamepad mode but shift/drop in Hybrid mode — this context-dependent behavior is hardcoded, not split into separate rebindable binds.
