# OpenStarbound: Steam Deck Build Guide

This document covers everything needed to compile and run OpenStarbound from source on a Steam Deck. Written after successfully building the project — all footguns and workarounds are documented.

## Environment Summary

| Item | Value |
|------|-------|
| **OS** | SteamOS 3.7.24 (Arch-based, kernel 6.11) |
| **CPU** | AMD custom APU, x86_64, 8 threads |
| **RAM** | 14 GB |
| **Build system** | distrobox (Ubuntu 22.04) + vcpkg + CMake + Ninja |
| **Build time** | ~30-40 min full clean build (after vcpkg cache is populated) |
| **Disk usage** | ~7 GB (build dir 3.5G + vcpkg 3.1G + cache 254M) |
| **Binary size** | 937 MB (RelWithDebInfo, unstripped) |

## Critical Context: SteamOS Immutable Root

**The Steam Deck's root filesystem is read-only.** This means:
- `/usr` is mounted read-only. You cannot `pacman -S` build tools.
- Runtime `.so` files exist (libGL, libX11, etc.) but `/usr/include` is empty — no headers.
- `steamos-readonly disable` exists but is fragile (wiped on SteamOS updates).
- **We build inside a distrobox container** (Ubuntu 22.04) which provides headers and compilers.
- **We run the built binary directly on the host** — it needs the host's GPU drivers.

The distrobox container shares `/home` with the host, so the source repo and build output are accessible from both environments.

## Prerequisites (Already Set Up)

The following are already configured on this machine. If starting fresh on a new Steam Deck, these are the setup steps:

### 1. Distrobox Container: `osb-build`

```bash
# Create (one-time)
distrobox create --name osb-build --image ubuntu:22.04 --yes

# Enter
distrobox enter osb-build
```

### 2. Build Dependencies (inside container)

```bash
sudo apt-get update

# Core build tools
sudo apt-get install -y build-essential ninja-build git curl zip unzip tar pkg-config python3-jinja2 python3-venv python3-pip

# CMake 4.x from Kitware (Ubuntu 22.04's default cmake 3.22 is too old)
sudo apt-get install -y ca-certificates gpg wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list
sudo apt-get update && sudo apt-get install -y cmake

# System dev libraries for SDL3, OpenGL, audio, Wayland, X11
sudo apt-get install -y \
  libxmu-dev libgl-dev libglu1-mesa-dev \
  libasound2-dev libpulse-dev libaudio-dev libjack-dev libsndio-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
  libxi-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev \
  libdbus-1-dev libibus-1.0-dev libudev-dev \
  libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev \
  autoconf autoconf-archive automake libtool patchelf
```

**FOOTGUN: `python3-venv` is required.** Without it, vcpkg's libsystemd build fails with a cryptic "Command failed: python3 -I -m venv" error. This isn't in the upstream CI because GitHub runners have it pre-installed.

### 3. vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT="$HOME/vcpkg"
```

## How to Build

### Quick Rebuild (after initial setup)

```bash
distrobox enter osb-build

export VCPKG_ROOT=/home/deck/vcpkg
cd ~/repos/OpenStarbound/source

# Incremental build (only recompiles changed files)
cmake --build --preset=linux-release
```

Incremental builds after changing a single file take 5-30 seconds.

### Full Clean Build

```bash
distrobox enter osb-build

export VCPKG_ROOT=/home/deck/vcpkg
cd ~/repos/OpenStarbound/source

# Remove old build
rm -rf ../build/linux-release

# Configure (triggers vcpkg dependency resolution — fast with cache)
cmake --preset=linux-release

# Build (all 8 cores)
cmake --build --preset=linux-release
```

### CMake Preset Details

The `linux-release` preset (from `source/CMakePresets.json`) configures:
- **Generator:** Ninja
- **Build type:** RelWithDebInfo (optimized + debug symbols)
- **Triplet:** `x64-linux-mixed` (static libs where possible, dynamic for Discord/Steam)
- **Static libgcc/libstdc++:** Yes (portable binary)
- **jemalloc:** Enabled
- **Steam + Discord integration:** Enabled
- **vcpkg toolchain:** Automatic via `$VCPKG_ROOT`

### Build Output

Binaries land in `~/repos/OpenStarbound/dist/`:

| Binary | Purpose |
|--------|---------|
| `starbound` | Game client (937 MB with debug info) |
| `starbound_server` | Dedicated server |
| `asset_packer` | Packs asset directories into .pak files |
| `asset_unpacker` | Unpacks .pak files |
| `btree_repacker` | Repacks world/save databases |
| `dump_versioned_json` / `make_versioned_json` | Versioned JSON utilities |
| `core_tests` / `game_tests` | Test binaries |

## Required Patch: GLEW Link Order Fix

**FOOTGUN:** The upstream `CMakeLists.txt` has a static linking bug on Linux where GLEW (static) references GLX symbols, but `${OPENGL_LIBRARY}` (containing libGLX) is linked *before* GLEW. With static libraries, linker order matters — symbols must be available *after* the library that references them.

The fix (already applied in our working tree):

```diff
diff --git a/source/CMakeLists.txt b/source/CMakeLists.txt
index e72bb17b..74ef95d2 100644
--- a/source/CMakeLists.txt
+++ b/source/CMakeLists.txt
@@ -537,8 +537,9 @@ if(STAR_BUILD_GUI)
 
   include_directories(SYSTEM ${GLEW_INCLUDE_DIR})
   set(STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS}
-      ${OPENGL_LIBRARY}
       $<IF:$<TARGET_EXISTS:GLEW::glew_s>,GLEW::glew_s,GLEW>
+      ${OPENGL_LIBRARY}
+      X11
   )
 
   if(STAR_SYSTEM_MACOS)
```

**What this does:** Moves GLEW before OpenGL in the link order (so GLX symbols are resolved for GLEW), and adds explicit `-lX11` (needed by GLX). Without this, you get `undefined reference to 'glXGetProcAddressARB'` at link time.

**If you `git checkout` or `git pull`:** You must re-apply this patch or the link step will fail. The compilation will succeed (all .o files build fine) — only the final link of the `starbound` binary fails.

## Runtime Configuration

### File Locations

| Path | Purpose |
|------|---------|
| `/home/deck/.local/share/Steam/steamapps/common/Starbound/assets/packed.pak` | Vanilla game assets (required) |
| `/home/deck/.local/share/Steam/steamapps/workshop/content/211820/` | Steam Workshop mods (28 installed) |
| `/home/deck/.local/share/Steam/steamapps/workshop/content/211820/729480149/` | Frackin Universe specifically |
| `/home/deck/repos/OpenStarbound/assets/opensb/` | OpenStarbound's own asset patches |
| `/home/deck/repos/OpenStarbound/dist/` | Built binaries + runtime config |

### sbinit.config

The runtime configuration lives at `dist/sbinit.config`:

```json
{
  "assetDirectories" : [
    "./assets/",
    "./mods/"
  ],

  "storageDirectory" : "./storage/",
  "logDirectory" : "./logs/"
}
```

The `assets/` directory contains:
- `packed.pak` — symlink to `/home/deck/.local/share/Steam/steamapps/common/Starbound/assets/packed.pak`
- `opensb.pak` — OpenStarbound's asset patches (packed from `assets/opensb/`)
- `user/` — symlink to vanilla user assets (playable songs)

Steam Workshop mods are loaded automatically via the Steam API (because `steam_appid.txt` is present), not via `sbinit.config`.

### Packing OpenStarbound Assets

**FOOTGUN:** You CANNOT point `assetDirectories` directly at the raw `assets/opensb/` directory. The asset loader will enumerate its subdirectories as individual asset sources and skip loose files at the root (like `preload.config` and `*.patch` files), causing a fatal crash.

You must pack them into a `.pak` file:

```bash
cd ~/repos/OpenStarbound
LD_LIBRARY_PATH=dist dist/asset_packer -c scripts/packing.config assets/opensb dist/assets/opensb.pak
```

**Re-pack after modifying any files in `assets/opensb/`** (asset patches, configs, UI layouts, etc.).

### Required Runtime Files in `dist/`

These must be present alongside the `starbound` binary:

- `libsteam_api.so` — copied from `lib/linux/`
- `libdiscord_game_sdk.so` — copied from `lib/linux/`
- `steam_appid.txt` — contains `211820` (Starbound's Steam app ID)
- `sbinit.config` — asset paths configuration
- `assets/opensb.pak` — packed OpenStarbound assets (see below)
- `assets/packed.pak` — symlink to vanilla Starbound assets

## Running the Game

**Run from the HOST, not inside distrobox.** The binary needs the host's GPU drivers.

```bash
cd ~/repos/OpenStarbound/dist
LD_LIBRARY_PATH=. ./starbound
```

The `LD_LIBRARY_PATH=.` ensures the Steam/Discord .so files in the current directory are found.

### Expected Behavior on First Launch

1. Asset loading takes 30-60 seconds on first run (parsing packed.pak + all Workshop mods)
2. Title screen appears
3. Subsequent launches are faster due to asset caching

### If It Crashes

Check `dist/logs/` for log files. Common issues:

| Symptom | Cause | Fix |
|---------|-------|-----|
| `libsteam_api.so: cannot open` | Missing .so in dist/ | `cp lib/linux/libsteam_api.so dist/` |
| `libdiscord_game_sdk.so: cannot open` | Missing .so in dist/ | `cp lib/linux/libdiscord_game_sdk.so dist/` |
| `libOpenGL.so.0: cannot open` | Running inside distrobox | Exit distrobox, run on host |
| `No available video device` | Running via SSH / no display | Must run locally in desktop mode, not over SSH |
| Asset loading errors | Wrong sbinit.config paths | Verify paths exist |
| `No such asset '/preload.config'` | Raw `assets/opensb/` dir used instead of packed .pak | Re-pack: `LD_LIBRARY_PATH=dist dist/asset_packer -c scripts/packing.config assets/opensb dist/assets/opensb.pak` |
| Segfault on startup | Missing `steam_appid.txt` | `cp scripts/steam_appid.txt dist/` |
| GL context creation failed | No display server | Must be in desktop mode (not game mode) or set `SDL_VIDEODRIVER` |

### Steam Integration

To get Steam overlay and Workshop mod auto-loading, either:
1. Keep `steam_appid.txt` (containing `211820`) in the `dist/` directory, OR
2. Add the built binary as a Non-Steam Game in Steam

## Development Workflow

### The Edit-Compile-Test Loop

```bash
# Terminal 1: Edit source files (on host or in distrobox — same filesystem)
# e.g., edit source/application/StarMainApplication_sdl.cpp

# Terminal 2 (in distrobox): Rebuild
distrobox enter osb-build
export VCPKG_ROOT=/home/deck/vcpkg
cd ~/repos/OpenStarbound/source
cmake --build --preset=linux-release

# Terminal 3 (on host): Run
cd ~/repos/OpenStarbound/dist
LD_LIBRARY_PATH=. ./starbound
```

### Source Code Layout

```
source/
├── CMakeLists.txt          # Top-level build config
├── extern/                 # Bundled: Lua 5.2, fmt, curve25519, xxhash, imgui bindings
├── core/                   # Core library: IO, crypto, JSON, Lua, networking, threading
├── base/                   # Asset loading, configuration, lighting, world geometry
├── platform/               # Platform abstraction interfaces
├── application/            # SDL3 window, input, OpenGL renderer, Steam/Discord integration
├── rendering/              # 2D rendering: tile painter, environment, text, drawables
├── windowing/              # GUI widget system: panes, buttons, lists, text boxes
├── frontend/               # Game UI: inventory, crafting, chat, options menus, voice
├── game/                   # Core game logic: entities, world, items, physics, scripting
├── client/                 # Client application entry point
├── server/                 # Server application entry point
├── utility/                # CLI tools (asset_packer, etc.)
└── test/                   # Google Test based tests
```

### Key Files for Controller Support

Based on our investigation, these are the most relevant files:

| File | Role |
|------|------|
| `source/core/StarInputEvent.hpp` | Input event types (already has controller events defined) |
| `source/application/StarMainApplication_sdl.cpp` | SDL3 event loop, gamepad init, stick/button processing |
| `source/game/StarInput.cpp` / `.hpp` | Input binding system, bind categories, action dispatch |
| `source/frontend/StarMainInterface.cpp` | Main game HUD, cursor/aim logic, `handleInputEvents()` |
| `source/windowing/StarPane.cpp` | Window/pane base class, mouse interaction |
| `source/windowing/StarPaneManager.cpp` | Manages pane focus, input routing |
| `source/frontend/StarBindingsMenu.cpp` | Keybindings configuration UI |
| `source/game/StarPlayer.cpp` | Player entity, tools, aim direction |
| `assets/opensb/` | Default keybinding configs, UI assets |

### Existing Controller Infrastructure

The codebase already has partial controller support:
- `SDL_CONTROLLERBUTTON*` / `SDL_CONTROLLERAXIS*` event types defined
- Controller button → bind mapping exists in the bind system
- `m_controllerInput`, `m_controllerLeftStick`, `m_controllerRightStick` member vars in the SDL application
- SDL3 gamepad init code present
- The Steam Deck's controls appear as an SDL gamepad via Steam Input

What's **missing** is the last-mile integration: analog stick → world-space aim, controller-aware menu navigation, and a coherent input mode switching system.

## Footgun Summary

1. **`python3-venv` must be installed** in the container before the first cmake configure. Omitting it causes a cryptic vcpkg build failure for libsystemd.

2. **CMake 3.22 (Ubuntu 22.04 default) is too old.** CMakePresets.json uses version 5 which requires CMake 3.24+. Install from Kitware's APT repo.

3. **GLEW link order patch is required.** Without it, linking fails with GLX undefined references. This is a genuine upstream bug that only manifests with the static GLEW + vcpkg toolchain on Linux.

4. **Always run the game binary on the host, never inside distrobox.** The container doesn't have GPU drivers. The binary is statically linked to libgcc/libstdc++ so it runs fine on the host.

5. **`LD_LIBRARY_PATH=.`** is required when running from `dist/` so the Steam/Discord .so files are found.

6. **RelWithDebInfo builds are huge** (937 MB for the client alone). This is because debug symbols are embedded. Do NOT strip them — they're essential for debugging controller support development. If disk is a concern, the build dir (3.5G) is the bigger target for cleanup.

7. **vcpkg first-run takes 15-30 minutes** to download and compile all dependencies. Subsequent configures are fast (cached). The cache lives at `~/.cache/vcpkg/`.

8. **After `git pull` or branch switches**, re-run the full configure + build:
   ```bash
   cd ~/repos/OpenStarbound/source
   cmake --preset=linux-release    # fast with vcpkg cache
   cmake --build --preset=linux-release
   ```
   And verify the GLEW link order patch is still applied.

10. **After modifying `assets/opensb/`**, re-pack the assets:
    ```bash
    cd ~/repos/OpenStarbound
    LD_LIBRARY_PATH=dist dist/asset_packer -c scripts/packing.config assets/opensb dist/assets/opensb.pak
    ```
    Forgetting this means the game will run with stale assets.

9. **The `VCPKG_ROOT` env var must be set** every time you enter the distrobox. Add to `~/.bashrc` inside the container for persistence:
   ```bash
   echo 'export VCPKG_ROOT=/home/deck/vcpkg' >> ~/.bashrc
   ```

## Disk Space

| Component | Size |
|-----------|------|
| Build directory (`build/linux-release/`) | 3.5 GB |
| vcpkg (`~/vcpkg/`) | 3.1 GB |
| vcpkg cache (`~/.cache/vcpkg/`) | 254 MB |
| dist/ (all binaries) | ~4.4 GB |
| **Total** | **~11 GB** |

Available on this device: ~72 GB (as of build completion).

## Useful Commands

```bash
# Enter build environment
distrobox enter osb-build

# Quick rebuild
export VCPKG_ROOT=/home/deck/vcpkg && cd ~/repos/OpenStarbound/source && cmake --build --preset=linux-release

# Re-pack opensb assets (only needed if you changed files in assets/opensb/)
cd ~/repos/OpenStarbound && LD_LIBRARY_PATH=dist dist/asset_packer -c scripts/packing.config assets/opensb dist/assets/opensb.pak

# Run game (from host)
cd ~/repos/OpenStarbound/dist && LD_LIBRARY_PATH=. ./starbound

# Run tests (from distrobox)
cd ~/repos/OpenStarbound/build/linux-release && ctest --label-regex NoAssets

# Check what the binary links against
ldd ~/repos/OpenStarbound/dist/starbound

# Pack OpenStarbound assets into a .pak (faster loading)
cd ~/repos/OpenStarbound && dist/asset_packer assets/opensb dist/opensb.pak

# View current CMake configuration
cmake --build --preset=linux-release -- -t help
```
