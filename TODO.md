# TODO

<!--toc:start-->
- [TODO](#todo)
  - [Gameplay and Features](#gameplay-and-features)
    - [Player and Controls](#player-and-controls)
    - [Interface](#interface)
    - [World and Objects](#world-and-objects)
    - [Characters, Items and Animation](#characters-items-and-animation)
  - [Modding and Scripting](#modding-and-scripting)
    - [Lua API and Tooling](#lua-api-and-tooling)
    - [Robustness and Safety](#robustness-and-safety)
  - [Multiplayer and Servers](#multiplayer-and-servers)
  - [Rendering and Performance](#rendering-and-performance)
    - [Directives and Images](#directives-and-images)
    - [Rendering and Fonts](#rendering-and-fonts)
    - [Engine Performance](#engine-performance)
  - [Build and Tooling](#build-and-tooling)
    - [Build System](#build-system)
    - [Warnings and Formatting](#warnings-and-formatting)
    - [Testing and Profiling](#testing-and-profiling)
  - [Cleanup and Tech Debt](#cleanup-and-tech-debt)
    - [Interfaces and Structure](#interfaces-and-structure)
    - [Hard-Coded Values](#hard-coded-values)
  - [Project and Community](#project-and-community)
  - [Not Planned](#not-planned)
  - [Sources of Ideas](#sources-of-ideas)
<!--toc:end-->

Every item is a paragraph or a few, and items are separated by `---`.
Items that came from another project say so in their text; see [Sources of Ideas](#sources-of-ideas).

## Gameplay and Features

### Player and Controls

**Controller support and remapping.** *(From the Xbox update, 1.4.5.)* The platform layer is there: `StarMainApplication_sdl.cpp` maps SDL gamepad axes and buttons and `StarInput.cpp` can bind controller events and report them to scripts.
Missing on top: menu navigation and a cursor driven by the controller, quick bar and inventory handling and a remapping UI.
Follow-ups: button prompts that follow the player's mapping (1.4.5.1), the quick bar showing the bound items, LB/RB item switching with long and short press and a setting for in-group or global switching and LT/RT to switch between inventory and storage (1.4.5.2).

---

**Snap mode, a precision mode for building.** *(From the Xbox update, 1.4.5.)* Could help mouse players as well as controller ones. See how it behaves in the Xbox build before designing it.

---

**Auto aim.** *(From the Xbox update, 1.4.5.)* Only matters with a controller. A script or an option once controller play works.

---

**Switch which hand is primary and which is secondary (dexterity).** A setting, per player, that makes left-handed play possible.

Today the left action bar slot is the primary hand and the right slot the secondary one (`PlayerInventory::primaryHeldItem` / `secondaryHeldItem`), and `Player` hands them to the `ToolUser` with `setItems(primary, secondary)` (`StarPlayer.cpp:1396`).

Left click starts primary fire and right click alt fire (`StarMainInterface.cpp:370-385`). The humanoid picks front and back arm from the primary and alt hand plus the facing direction (`StarHumanoid.cpp:949`).

Simplest approach: swap the mouse buttons at the `MainInterface` input, which also covers techs and the wire tool. A fuller one swaps the items passed to `setItems`, which the arms, `Player::primaryHandItem()` and the Lua callbacks would then follow.

Open questions: two-handed items always sit in the primary slot, and the action bar's left and right click behavior, the legacy action bar and scripts reading `primaryHandItem` would need to agree.

---

**Night vision toggle.** *(From StarExtensions.)*

---

### Interface

Voice "Players" button in the options menu is disabled and still labelled TODO (`assets/opensb/interface/optionsmenu/optionsmenu.config.patch`).

---

Add an instrument volume range to the options menu or share one range across all three sliders (`source/frontend/StarOptionsMenu.hpp`).

---

Spinner widget (`source/windowing/StarSliderBar.cpp:39`).

---

Allow viewing other recipes without interrupting crafting (`source/frontend/StarCraftingInterface.cpp:252`).

---

**Recipes with a zero-count ingredient need two clicks to craft, or the quantity has to be re-set to x1.** The quantity box seems to start at x0, so the first craft does nothing. Caused by recipes that need 0 of an ingredient (always craftable)? Not reproduced yet.

Lead: `ItemDatabase::maxCraftableInBag` (`source/game/StarItemDatabase.cpp:302`) starts from `highest<uint64_t>()` and only lowers it for inputs and currencies with a count above zero, so a recipe whose inputs are all zero returns `UINT64_MAX`. `CraftingPane::maxCraft` (`source/frontend/StarCraftingInterface.cpp:754`) stores that in an `int`, which wraps to a negative number and that value is then used for the quantity buttons (lines 49-60) and the clamp in `countTextChanged` (line 608). With the upper bound at -1, `clamp(value, 1, maxCraft())` has its bounds the wrong way round, which could leave the count at 0 or below.

Expected fix: make `maxCraft` return at least 1, or cap the result at `m_maxSpinCount` before it goes into an `int` and check what the quantity box shows for a recipe that cannot be crafted at all (it is fine for it to read x1 there, since the craft button is disabled and nothing else uses it).

---

**Click-through mask for panes (instead of non-rectangular panes).** Panes are drawn from header, body and footer images (`Pane::setBG`), so transparent corners already look right, but hit testing is a plain rectangle: `Pane::inWindow` uses `screenBoundRect()`, and `PaneManager` routes mouse events with it (`StarPaneManager.cpp:114,126`). Clicks on see-through corners are swallowed by the pane and it drags from empty space.

Making panes themselves non-rectangular (a shape that layout, children, dragging and bounds all understand) is a big change and the wrong tool. A pane stays a rectangle, and a mask parameter in its config says which parts of it can be clicked and which let the click pass through to what is behind.

The mask can be a separate image or a list of rects or polygons in the pane config. `Pane::inWindow` (and `inDragArea`) would test the cursor against it, and `PaneManager` would then offer the event to the next pane underneath. A pane without a mask keeps the plain rectangle check and pays nothing. A mask is explicit and cheap to test, and it is independent of the art, so semi-transparent but solid parts (shadows, glows) can still catch clicks, unlike an alpha check.

Open questions: whether a pane's children outside the clickable area still get events (they probably should) and whether the mask should apply to hover and tooltips as well as clicks (probably).

---

**Images in text messages, `^image=/asset.png;`.** *(From the Xbox update, 1.4.5.)* The Xbox update lets a chat message carry an asset image through a text command. Text commands are `^...;` sequences handled in `Text::processText` (`source/core/StarText.cpp`, `CmdEsc`, with `stripEscapeCodes` for plain text), which already covers colors like `^red;`, so the command itself fits there as a new one.

Idea to reuse the emoji handling: emoji are drawn as glyphs from the emoji font, and `FontTextureGroup` and `TextPainter` already lay out, wrap and cache those glyphs by codepoint. An inline image could be given a private-use codepoint that maps to a texture and a width, so layout, wrapping and the glyph cache stay as they are, and only the glyph lookup and the draw step know about it. Whether that is simpler than drawing images separately depends on how the painter measures a glyph (`glyphWidth`) and caches it per font size.

Things to decide: the image size (scale to the line height, with a limit on the width), what the path may contain (a message comes from other players, and a path with `?` directives can be expensive to build, as the directive work showed, so allow only plain asset paths or cap the directive chain), what `stripEscapeCodes`, the chat log and the chat bubbles show for it, and what happens to a path that does not exist (the placeholder, not an error, see the item about crashes on missing assets).

Old clients should just strip an unknown `^...;` command, so they would see the message without the image. Check that before relying on it.

---

**Item drop tooltips on hover** and **chat opacity and directive customization.** *(From StarExtensions.)*

---

**Text-to-speech.** *(From StarExtensions, which uses Software Automatic Mouth.)* A lot of build and licensing (?) work for little, so low priority.

---

### World and Objects

**Per-pixel object positions, with overlap checks on a bitmask.** Objects sit on whole tiles today (`Object::tilePosition`, `Object::spaces`) and which tiles an object occupies comes from its `spaces` list plus, if `spaceScan` is set, the tiles whose image coverage passes that threshold (`StarObjectDatabase.cpp:194`, `ImageMetadataDatabase::imageSpaces`, counted per 8x8 pixel tile). Idea: let an object have a position in pixels, and decide whether two objects overlap by comparing bitmasks of their pixels.

Old clients, which know nothing about the pixel offset, would see the object rounded to the nearest tile.

Some objects that did not overlap before would overlap, because pixels in a corner of a tile that vanilla's space scan does not count (for example a stepped, diagonal object that vanilla treats as three tiles) could now conflict. To avoid that, the bitmask has to be `&`-ed with the `spaceScan` result, so the scan only ever removes pixels from the mask and a placement vanilla allowed is still allowed.

Open questions: how the pixel offset is stored and sent (a new field in the object's network state, which old clients ignore), what the mask is built from (the image, as `spaceScan` does, or an explicit one in the object config), and the cost of the overlap test for crowded areas.

---

**Wider material, matmod and liquid ids (32-bit), assigned at runtime from string names.** Ids are small fixed-width numbers that every asset has to choose by hand: `MaterialId` and `ModId` are `uint16_t` with the top of the range reserved (`StarMaterialTypes.hpp`: `EmptyMaterialId` 65535, the meta materials from 65000, `NoModId` 65535, `BiomeModId` 65534), and `LiquidId` is a `uint8_t` (`StarLiquidTypes.hpp`, so at most 255). Big mod packs run into these limits and have to coordinate ids with each other.

Idea, as Minecraft does it: asset authors only give string names, and numeric ids are assigned when the game starts. The client gets the server's assignment when it joins, so both sides agree. World data would be saved with the string names in a lookup table inside the chunk or region, and each tile would store its index into that table, so a world does not depend on the order mods happened to load in.

The hard part is staying compatible. Vanilla clients that join a modded server would see a block, liquid or matmod they have no id for, so those need to draw as some fallback asset (still to decide which, and whether it is a placeholder per kind). Existing worlds and the current `materialId` / `liquidId` values in asset files must keep loading, so explicit ids in assets would still win and the runtime assignment would fill in the rest.

Open questions: the tile storage format (what a tile takes up with 32-bit ids, and how the lookup table is versioned in the world file and in the network protocol), what happens to scripts and mods that use the numeric ids in Lua, and how many places assume 16 bits (`ServerTile`, the tile network packets, `TileDamage`, the lighting and liquid code).

---

Call something on the world script after tile modification (`source/game/StarWorldClient.cpp:1105`) and stop assuming every tile modification was caused by the player (line 1044).

---

**Worldgen performance.** *(From the Xbox update, 1.4.5.)* The changelog only says "performance updates surrounding the world generation system". Worth comparing if someone has the 1.4.5 build to diff.

---

Platformer A* should avoid damaging liquids (`StarPlatformerAStar.cpp:261`).

---

### Characters, Items and Animation

**Humanoid breathing animation and chest dynamics.** *(From StarExtensions.)* Neither is in the humanoid code here (head rotation is, `Humanoid::setHeadRotation`). Needs frames or config for the offsets, so it should be data-driven.

---

**Better sprite scaling for techs and status effects.** *(From StarExtensions.)* Beautifies tech and status scaling by changing how it's rendered to scale the drawables instead of the sprites. Works with parent directives set from tech and status effect scripts, and primary directives from status primary scripts! This also fixes various offset issues with scaling, such as your head detaching when you crouch..

---

Non-aiming item holding isn't supported in the humanoid (`source/game/StarHumanoid.cpp:1808,1843`); make the hard-coded value at line 1545 configurable.

---

Make beam item particle cooldown configurable from JSON (`source/game/interfaces/StarBeamItem.cpp:231`).

---

## Modding and Scripting

### Lua API and Tooling

**A VS Code extension that shows the documentation of the Lua API.** The stubs already exist: `doc/lua/stubs/` has 36 files with LuaLS annotations (`---@meta`, `---@param`, `---@return`) next to the 34 markdown pages in `doc/lua/`, and `doc/lua/stubs/README.md` explains how to use them with the Lua extension from sumneko. That setup is manual: install the Lua extension, clone the repository, and put the absolute path of `doc/lua/stubs` in `Lua.workspace.library`.

Idea: an extension that bundles the stubs, adds them to the library setting by itself, and so gives hover documentation, signature help and completion for the Starbound API without any setup, and is updated with the game's releases. It could depend on the sumneko extension, or ship its own small provider for hover and completion from the markdown if that is easier to keep correct.

Make it aware of which script it is in, which the plain stubs cannot be: an object script gets `object` and `animator`, a status effect gets `effect`, an ActiveItem script gets `activeItem`, and so on, so that functions from the wrong context are not offered. The script type is known from the `scripts` lists in the `.object`, `.activeitem`, `.statuseffect` and similar config files that refer to the script.

Also: completion and checking of asset paths in the config files, jumping from a call to its page in `doc/lua/`, and a way to keep the stubs and the markdown pages from drifting apart (some stubs still say `TODO: Add description`, and `songbook.keepAlive` and `songbook.play` are undocumented, see the songbook item below).

Open questions: where it lives (this repository, so the stubs are in step with the code, or its own), and publishing it to the Marketplace and Open VSX.

---

Document `songbook.keepAlive` and `songbook.play` (`doc/lua/openstarbound/songbook.md`, `doc/lua/stubs/openstarbound/songbook.lua`).

---

**Isolated virtual persistent storage for mods.** Give each mod its own sandboxed, persistent key/value or file store, so mods can save data without needing `safeScripts` turned off.

---

### Robustness and Safety

**Stop crashing on errors that could be logged and skipped.** A missing status effect, animation or similar asset reference takes the whole game down, where the harm of ignoring it is one effect not showing. For example, `StatusEffectDatabase` throws `No such unique stat effect '...'` (`StarStatusEffectDatabase.cpp:29`), and an exception that is not caught near where it happens goes up to the main loop and ends the session. Mods hit this most, since they refer to each other's assets.

Idea: go through the places that look up an asset by name from data (status effects, animations and animated parts, particles, sounds, item and object references, tech and effect scripts), and for each one decide between failing and falling back. Where an error cannot corrupt anything, log it once with the asset and the source that referred to it, use a harmless default (no effect, the missing-asset placeholder, silence) and carry on.

Do not hide errors that mean the data is really broken, such as world storage or network protocol problems, and keep the log message loud enough that a missing asset is still found. Rate-limit it so a missing asset that is looked up every frame does not flood the log.

Open questions: whether a catch at the entity update level (log, then drop or disable that entity) is simpler and safer than fixing each lookup, and what the placeholder for each kind of asset should be.

---

**Protecting items and portraits from being copied by scripts.** *(From StarExtensions.)* Find out what a script can currently get of another player's items and portrait on a server (for example through the entity and portrait callbacks), what the extension actually blocks and whether a server or a player setting should decide.

---

## Multiplayer and Servers

Server query thread: two unimplemented handlers (`source/server/StarServerQueryThread.cpp:67,80`).

---

Network full world protection directly to newer clients (`source/game/StarGameTypes.hpp:188`).

---

Write full player data less often in the team client (`source/game/StarTeamClient.cpp:200`).

---

**Fixes StarExtensions lists, to check against OSB** (reproduce first): crash when disconnecting with the inventory open, chat color lost when wrapping text, audio problems when joining while a song plays, and the connection error screen disappearing.

---

## Rendering and Performance

### Directives and Images

Directives: move to `+=` in `StarParallax.cpp:75`; improve the quick fix in `StarAssets.cpp:1536`.

---

**Directives still worth a look for speed.** Found by replaying the paths in `storage/slow-textures.txt` after #652. A world load still spends a few seconds in these, mostly parallax, sky and plant images.

Parallax with a smooth scale, e.g. `/parallax/images/cityscape0/base/1.png?brightness=50?hueshift=20?scale=2` (126 ms, 103 ms of it image work). A bilinear enlarge does not just copy pixels, so the color operations cannot be moved after it the way they are for `?scalenearest`. First find out whether the scale or the colors cost more.

The same image with `?flipy?brightness=90?scalenearest=7?multiply=FFFFFFAA` took 525 ms in an earlier log, before the scale and color reorder and the exact fast kernels. Check it again in a world that uses it.

Sky and plant color chains, 20-48 ms each and many per world: `alienback/base/1.png?brightness=20?saturation=-10?hueshift=-15?fade=ffffff=0.15` (48 ms), the `alienbigleaf` trees with `?saturation?brightness?hueshift=0?fade`, and the `clouds*` images with `?hueshift=-15`. What is left is one pass per operation. Ideas: fuse a run of per-pixel operations into one pass with a single small color cache (the `RecentColors` idea for the whole chain), or convert only the distinct colors of an image that has few of them.

Costume drawables with long `?scale=...?crop=...?replace;...` chains, e.g. the avian `bsleeve` and froggmerchant `pantsM` (200-250 ms each in an earlier log). `?replace` is cheap now, so look at what the scales and crops cost.

`?addmask` on celestial images, e.g. `moon/maskie1.png?hueshift=-45?addmask=...` (about 22 ms each).

`?blendmult` / `?blendscreen` still convert every pixel in the box to float and back. That is cheap for sign tiles, but a blend of two full-size images (e.g. `hazardtapeh` with a `blendscreen` of an `oceanglow` parallax, 21 ms) could use an integer or lookup path if it gives the same bytes.

---

**Generated clothes performance by baking the directives.** *(From StarExtensions.)* The extension turns the long directive chains of generated clothes into images. This lines up with the costume drawables that are slow to load (long `?scale`, `?crop` and `?replace` chains, see the directive speed item above), so it might make those cheaper than any per-operation speedup. Needs a look at where and when the baked image would be stored and how it is invalidated.

---

**Compare more directives with the Windows build of Starbound.** `?hueshift`, `?saturation`, `?brightness`, `?fade` and `?scanlines` were checked against the Windows (MSVC) and Linux vanilla binaries and match the Windows output. Not yet compared against the Windows binary: `?replace`, `?addmask`, `?blendmult` / `?blendscreen`, `?border`. A full ARM64 build of `StarColor.cpp` has not been tried either.

---

### Rendering and Fonts

Cloud shake when interpolating between decimal zoom levels (`source/rendering/StarEnvironmentPainter.cpp:237`).

---

Texture atlas: better handling when locked (`source/application/StarTextureAtlas.hpp:264`).

---

**Optimize drawables.** Reduce per-frame cost of building, copying and painting `Drawable` lists (see `StarWorldPainter.cpp`, where entity drawables are regrouped and moved each frame).

---

**Lighting is the biggest cost when building.** A `perf` profile of the building scenario, taken while working on the directive speedups, showed the lighting update dominating the frame, well ahead of the directives. The numbers were not kept, so profile it again before choosing what to change (`StarLightmap` and the world client's lighting thread are where to start).

---

**Let the font config describe the characters a font handles.** `FontTextureGroup::getFontForCharacter` (`source/rendering/StarFontTextureGroup.cpp`) decides what goes to the emoji font with a hard-coded list of Unicode ranges, which only that one font gets. Some of the ranges overlap or repeat each other (`0xFE00-0xFE0F` and `65024-65039` are the same range, and `0x1F018-0x1F270` covers parts of others), and a mod cannot change any of it.

Idea: let the `font` block in `/interface.config` (which already names `defaultFont`, `fallbackFont` and `emojiFont`, read in `TextPainter::reloadFonts`) say which character ranges each font wants, so the lookup walks a list of `{ font, ranges }` entries instead of special-casing the emoji font. The current ranges become the default config, and a font with no ranges keeps today's behavior (the active font, then the fallback).

Open questions: the order when ranges overlap, whether a font still has to `exists(c)` the character to be chosen (it does today) and how the lookup stays cheap per character (a sorted range table, not the chain of comparisons).

---

**A `/render` command** that saves an image of what the game draws. *(From StarExtensions.)*

---

### Engine Performance

`Vec` inefficiency (`source/core/StarVector.hpp:639`); `StringView` conversions and speed (`StarGuiContext.cpp:407`, `StarStringView.cpp:301`).

---

Weighted pool is O(n), could be O(log n) (`StarWeightedPool.hpp:177`); B-tree remove and shifting are slow (`StarBTree.hpp:144`, `StarBTreeDatabase.cpp:699`).

---

Wire processor is recursive depth-first (`StarWireProcessor.cpp:70`); weather update complexity (`StarWeather.cpp:204`).

---

Voice buffer should be a ring buffer (`source/frontend/StarVoice.cpp:66`).

---

## Build and Tooling

### Build System

**Build with the system's libraries instead of vcpkg (low priority, for later).** Suggested by a contributor who wanted to build and test on FreeBSD but found the vcpkg setup too cumbersome. vcpkg does not officially support every platform, and most Unix-like systems that are not Linux (the BSDs, for example) already package every dependency.

Today `source/CMakeLists.txt` finds most libraries in `CONFIG` mode (`zlib-ng`, `zstd`, `cpr`, `Opus`, `re2`, `cpptrace`, `SDL3`, `mimalloc`, and `imgui` unless `STAR_USE_VENDORED_IMGUI` is on), which is the form vcpkg installs but distribution packages often do not ship. The manifest is `source/vcpkg.json`, and `README.md` makes vcpkg a required step.

The Nix package (`nix/package.nix`) already builds against the system's libraries, so it shows how much has to give: it overrides imgui to a newer version with the SDL3, OpenGL3 and FreeType bindings and patches a `find_package(freetype CONFIG)` call. Those patches are a list of what a system-library mode would need to handle itself.

Idea: a CMake option (for example `STAR_USE_SYSTEM_LIBS`) that falls back from `CONFIG` to module or `pkg-config` lookups for each dependency, documented with the package names for the common distributions and for FreeBSD. Then try a FreeBSD build and see what else breaks (other than the libraries, the platform code in `*_unix.cpp`).

---

### Warnings and Formatting

**Basic warning cleanup.** A clean build with GCC 16.2.0 (the Nix dev build) prints about 400 warning lines, which is enough to hide a new one.

About 350 of them come from `source/extern`, almost all one `-Wtautological-compare` in `fmt/ranges.h` repeated for every file that includes it. Silence that by marking the extern include directories as system includes, or by a warning pragma around the include, and leave the vendored code alone.

The rest is about two dozen distinct warnings in our own code: `-Wmaybe-uninitialized` (11, many through `StarMaybe.hpp`), `-Wsign-compare` (4), `-Warray-bounds` (4, partly through `StarRect.hpp`), and one each of `-Wreturn-type`, `-Wpessimizing-move`, `-Wint-in-bool-context`, `-Wdangling-reference` and `-Wclass-memaccess` (the `replaceColors` item below). Fix them one by one, and check with clang as well, since each compiler reports different ones.

Then make new warnings visible, for example `-Werror` for a short list of warnings in CI once the log is clean.

---

`replaceColors` in `source/core/StarImageProcessing.cpp` copies a `uint32_t` into a `Vec4B` with `memcpy`, which GCC reports as `-Wclass-memaccess` (non-trivial type). Use a plain array or a union-free copy so the build log stays clean.

---

**Formatting (low priority, hold off).** `.clang-format` exists and the pre-commit hook (`scripts/git-hooks/pre-commit`, #653) rejects staged C++ files that are not clang-format clean, but most of the existing code was never formatted (measured with clang-format 21.1.8: 1032 of the 1059 C++ files outside `source/extern` are not clean, and formatting them would change about 52,000 lines of 230,000), so touching a file means formatting all of it or failing the hook. A full-source reformat is not planned for now: it would conflict with every open PR and branch. If it is ever done, `scripts/format-source.sh` formats the tree, and it should be one commit with nothing else in it, listed in a `.git-blame-ignore-revs` file so `git blame` keeps the real author of each line.

---

### Testing and Profiling

**Test that directives give the same bytes on every compiler.** The color directives (`?hueshift`, `?saturation`, `?brightness`, `?fade`, `?scanlines`) were written so GCC, clang (x86 and ARM64) and MSVC all produce the Windows build's output, which depends on keeping fast-math and fused multiply-adds out of that code (`StarColor.cpp`). Nothing in the repository checks it, so a flag or compiler change could shift single pixels again without anyone noticing. Add a test with hashes of the output for fixed images and parameters (a few thousand colors times a handful of parameters, plus the random chains used for the speedups, which stay in `deconstruction/costume-directives/bench/`) and run it in the CI matrix.

---

**Keep the texture-load timing as an opt-in tool.** While looking for slow directives, a development build logged every texture load that took more than a few milliseconds (path, time split into image work and texture creation), plus per-interval texture-cache and loop statistics. It made the work above possible and nothing like it exists upstream. If it goes in, make it off by default, keep the paths out of the normal log (they can be megabytes long) and cap or rotate the file it writes. The patch is kept in `deconstruction/costume-directives/instrumentation/` next to the benchmarks.

---

## Cleanup and Tech Debt

### Interfaces and Structure

`ToolUserEntity` interface cleanup (`source/game/interfaces/StarToolUserEntity.hpp`, `StarToolUserItem.hpp`, `StarToolUser.hpp:42`).

---

Thread safety of `UniverseClient` (`StarUniverseClient.hpp:17`).

---

### Hard-Coded Values

Hard-coded icon size 16 and slot progress frames in item slot rendering (`source/game/StarItem.cpp:241,260`, `source/windowing/StarItemSlotWidget.cpp:196,211`).

---

Hard-coded scroll area values (`source/windowing/StarScrollArea.cpp:7`).

---

## Project and Community

**An automatic credits page with the GitHub usernames of everyone who contributed to OpenStarbound.** A page at the end of the game's credits, built from the project's contributors instead of a hand-kept list that falls behind. The vanilla credits are not code but a timed cinematic: the end of `/cinematics/story/finale/finale.cinematic` (from about 126 s, with texts such as `STARBOUND TEAM` and `SPECIAL THANKS` and picture slides from `/cinematics/story/credits/`), which the eye boss death behavior starts (`behaviors/monsters/boss/eyeboss/eyebossdeath.behavior`). Nothing in this repository adds to it. (Read from a local copy of the vanilla assets in `attic/chucklefish`, which is git-ignored.)

Idea: generate the list when a release is built, from the GitHub contributors of the repository (the API's contributors endpoint) or from `git log`, write it to an asset (for example a JSON list in `assets/opensb`), and add a cinematic part that shows that list after the existing credits, for example with a patch of `finale.cinematic` that appends text entries with their own `startTime` and `endTime`. A very long list would need to be split over several entries or scrolled. Generating it at build time keeps the game from needing the network, and the release job is the place that can reach GitHub.

Things to decide: `git log` gives names and emails and not GitHub usernames, so the API (or the `Co-authored-by` and noreply addresses) is needed to get usernames. Bots such as dependency updaters should be left out. People should be able to ask to be left out or listed differently, since a username is public but not everyone wants it in a game. Merged PRs squashed into one commit keep the author (as with #652), but co-authors need the trailers.

Not checked: whether the credits can also be opened from somewhere else (the title screen, a command), which would be the better place for a long list than a cinematic that only plays after the boss.

---

## Not Planned

The balance changes, the Xbox-only fixes and moving the quest tracker and boss health bar from the Xbox update (1.4.5), which would change layouts mods depend on.

---

Already here in some form, so nothing to port from StarExtensions: custom binds (`assets/opensb/binds/opensb.binds`), tile prediction (`WorldClient::informTilePrediction`), vapor trail (`Humanoid::setVaporTrail`), head rotation, identity editing, chat emotes and the Lua bindings for modders.

---

## Sources of Ideas

What is worth importing from the [Xbox 1.4.5 changelog](https://starbounder.org/Version_1.4.5) (1.4.5, 1.4.5.1 and 1.4.5.2).
Only the changelog text was read since I don't have the assets or binary.

---

What is worth porting from [StarExtensions](https://github.com/XXXXDDDDD/StarExtensions).
It's closed source, so each item comes from the description.
