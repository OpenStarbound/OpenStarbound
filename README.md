# OpenStarbound

This is a fork of Starbound. Contributions are welcome!
You **must** own a copy of Starbound to use it. Base game assets are not provided for obvious reasons.

It is **work-in-progress**, so up-to-date compiled builds for all platforms are not yet available. GitHub Actions will likely be set up to provide compiled builds in the future.

However, a manually compiled client build (Nov 30th, 2023) is available [here](https://files.catbox.moe/izkhxa.zip).
Please note that this is a **development** build. It is not considered stable and should not be used in normal play.
Extract it to a new `win_opensb` folder in your Starbound installation, adjacent to the win32 & win64 folders.

If you do decide to compile it anyways, you must also make sure it loads the game assets in `/assets/opensb/`.

Changes:
* You can now make .patch files that are just merged in. That's why the patch files in here are unusually simple.
* Some Lua functions like getters and setters for player identity values, but not every function from [StarExtensions](https://github.com/StarExtensions/StarExtensions) has been ported yet.
* Character swapping (rewrite from StarExtensions, currently command-only: `/swap name` case-insensitive, only substring required)
* Custom user input support with a keybindings menu (rewrite from StarExtensions)
* Positional Voice Chat that works on completely vanilla servers, uses Opus for crisp, HD audio (rewrite from StarExtensions)
  * Both menus are made available in the options menu in this fork rather than as a chat command.
* Multiple font support (switch fonts inline with `^font=name;`, .ttf assets are auto-detected)
* World lightmap generation has been moved off the main thread
* Experimental changes to the storage of directives in memory to greatly reduce their impact on frametimes
  * Works well when extremely long directives are used for "vanilla multiplayer-compatible" creations, like [generated clothing](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/).

* Client-side tile placement prediction (rewrite from StarExtensions)
  * You can also resize the placement area of tiles on the fly.
* Support for placing foreground tiles with a custom collision type (rewrite from StarExtensions, requires OpenSB server)
  * Additionally, objects can be placed under non-solid foreground tiles.

* Some minor polish to UI

[Discord](https://discord.gg/D4QqtBNmAY)
