# TODO

<!--toc:start-->
- [TODO](#todo)
  - [Planned](#planned)
    - [Lumi](#lumi)
  - [Found in the Code](#found-in-the-code)
    - [Features and Unfinished UI](#features-and-unfinished-ui)
    - [Rendering and Performance](#rendering-and-performance)
    - [Cleanup and tech debt](#cleanup-and-tech-debt)
<!--toc:end-->

## Planned

### Lumi

- [ ] **Isolated virtual persistent storage for mods.** Give each mod its own sandboxed, persistent key/value or file store, so mods can save data without needing `safeScripts` turned off.
- [ ] **Optimize drawables.** Reduce per-frame cost of building, copying and painting `Drawable` lists (see `StarWorldPainter.cpp`, where entity drawables are regrouped and moved each frame).
- [ ] **Scale chat bubbles with the UI scale.** `StarChatBubbleManager.cpp` mixes `pixelRatio` and `m_zoom` when sizing text and padding; make bubble size, padding and wrap width follow the interface scale.

## Found in the Code

### Features and Unfinished UI

- [ ] Voice "Players" button in the options menu is disabled and still labelled TODO (`assets/opensb/interface/optionsmenu/optionsmenu.config.patch`).
- [ ] Add an instrument volume range to the options menu or share one range across all three sliders (`source/frontend/StarOptionsMenu.hpp`).
- [ ] Document `songbook.keepAlive` and `songbook.play` (`doc/lua/openstarbound/songbook.md`, `doc/lua/stubs/openstarbound/songbook.lua`).
- [ ] Server query thread: two unimplemented handlers (`source/server/StarServerQueryThread.cpp:67,80`).
- [ ] Non-aiming item holding isn't supported in the humanoid (`source/game/StarHumanoid.cpp:1808,1843`); make the hard-coded value at line 1545 configurable.
- [ ] Make beam item particle cooldown configurable from JSON (`source/game/interfaces/StarBeamItem.cpp:231`).
- [ ] Allow viewing other recipes without interrupting crafting (`source/frontend/StarCraftingInterface.cpp:252`).
- [ ] Spinner widget (`source/windowing/StarSliderBar.cpp:39`).
- [ ] Network full world protection directly to newer clients (`source/game/StarGameTypes.hpp:188`).
- [ ] Write full player data less often in the team client (`source/game/StarTeamClient.cpp:200`).
- [ ] Call something on the world script after tile modification (`source/game/StarWorldClient.cpp:1105`) and stop assuming every tile modification was caused by the player (line 1044).

### Rendering and Performance

- [ ] Cloud shake when interpolating between decimal zoom levels (`source/rendering/StarEnvironmentPainter.cpp:237`).
- [ ] Texture atlas: better handling when locked (`source/application/StarTextureAtlas.hpp:264`).
- [ ] Hard-coded icon size 16 and slot progress frames in item slot rendering (`source/game/StarItem.cpp:241,260`, `source/windowing/StarItemSlotWidget.cpp:196,211`).
- [ ] `Vec` inefficiency (`source/core/StarVector.hpp:639`); `StringView` conversions and speed (`StarGuiContext.cpp:407`, `StarStringView.cpp:301`).
- [ ] Directives: move to `+=` in `StarParallax.cpp:75`; improve the quick fix in `StarAssets.cpp:1333`.
- [ ] Voice buffer should be a ring buffer (`source/frontend/StarVoice.cpp:66`).

### Cleanup and Tech Debt

- [ ] Rewrite the chat bubble sizing code, which the comment calls the ugliest in the codebase (`StarChatBubbleManager.cpp:199`). Combine with the chat bubble scaling item above.
- [ ] `ToolUserEntity` interface cleanup (`source/game/interfaces/StarToolUserEntity.hpp`, `StarToolUserItem.hpp`, `StarToolUser.hpp:42`).
- [ ] Hard-coded scroll area values (`source/windowing/StarScrollArea.cpp:7`).
- [ ] Thread safety of `UniverseClient` (`StarUniverseClient.hpp:17`).
- [ ] Weighted pool is O(n), could be O(log n) (`StarWeightedPool.hpp:177`); B-tree remove and shifting are slow (`StarBTree.hpp:144`, `StarBTreeDatabase.cpp:699`).
- [ ] Wire processor is recursive depth-first (`StarWireProcessor.cpp:70`); weather update complexity (`StarWeather.cpp:204`).
- [ ] Platformer A* should avoid damaging liquids (`StarPlatformerAStar.cpp:261`).
