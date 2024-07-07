# OpenStarbound

This is a fork of Starbound. Contributions are welcome!
You **must** own a copy of Starbound to use it. Base game assets are not provided for obvious reasons.

It is still **work-in-progress**.

## Installation
You can download a nightly build below, or the [latest release](https://github.com/OpenStarbound/OpenStarbound/releases/latest). At the moment, you must copy the game assets (**packed.pak**) from your normal Starbound install to the OpenStarbound assets directory before playing.

An installer is available for Windows. otherwise, extract the client/server zip for your platform and copy the game assets (packed.pak) to the OpenStarbound assets folder. the macOS releases currently lack the sbinit.config and folder structure that the Linux & Windows zips have, so you'll need to create those before running them. For macOS releases, it is recommended to build them from source (See guide below).
### Nightly Builds
These link directly to the latest build from the [Actions](https://github.com/OpenStarbound/OpenStarbound/actions?query=branch%3Amain) tab.

[**Windows**](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_windows/main):
[Installer](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_windows/main/Installer.zip),
[Client](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_windows/main/OpenStarbound-Windows-Client.zip),
[Server](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_windows/main/OpenStarbound-Windows-Server.zip)

[**Linux**](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_linux/main):
[Client](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_linux/main/OpenStarbound-Linux-Client.zip),
[Server](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_linux/main/OpenStarbound-Linux-Server.zip)

[**macOS**](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_macos/main "overpriced aluminium"): 
[Intel](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_macos/main/OpenStarbound-Dev-macOS-Intel.zip),
[ARM](https://nightly.link/OpenStarbound/OpenStarbound/workflows/build_macos/main/OpenStarbound-Dev-macOS-Silicon.zip)

## Changes
Note: Not every function from [StarExtensions](https://github.com/StarExtensions/StarExtensions) has been ported yet, but near-full compatibility with mods that use StarExtensions features is planned.

### Lighting
**The lightmap generation has been moved off the main thread, and supports higher color range.**
  * Point lights are now additive, which is more accurate - you'll notice that different lights mix together better!
  * Object spread lights are auto-converted to a hybrid light which is 25% additive.

### Assets
* Assets can now run Lua scripts on load, and after all sources have been loaded.
  * These scripts can modify, read, patch and create new assets!
* Lua patch files now exist - **.patch.lua**
  * These can patch JSON assets, as well as images!
### Misc
* Player functions for saving/loading, modifying the humanoid identity
* Character swapping (rewrite from StarExtensions, currently command-only: `/swap name` case-insensitive, only substring required)
* Custom user input support with a keybindings menu (rewrite from StarExtensions)
* Positional Voice Chat that works on completely vanilla servers, uses Opus for crisp, HD audio (rewrite from StarExtensions)
  * Both menus are made available in the options menu in this fork rather than as a chat command.
* Multiple font support (switch fonts inline with `^font=name;`, **.ttf** and **.woff2** assets are auto-detected)
  * **.woff2** fonts are much smaller than **.ttf**, [here's a web conversion tool](https://kombu.kanejaku.org/)!
* Experimental changes to the storage of directives in memory to reduce copying - can reduce their impact on frametimes when very long directives are present
  * Works especially well when extremely long directives are used for "vanilla multiplayer-compatible" creations, like [generated clothing](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) or custom items/objects.

* Client-side tile placement prediction (rewrite from StarExtensions)
  * You can also resize the placement area of tiles on the fly.
* Support for placing foreground tiles with a custom collision type (rewrite from StarExtensions, requires OpenSB server)
  * Additionally, objects can be placed under non-solid foreground tiles.

* Some minor polish to UI
* The Skybox's sun now matches the system type you're currently in.
  * Previously generated planets will not have this feature and will display the default sun.
  * Modded system types require a patch to display their custom sun.
  * You can also access the skybox sun scale and its default ray colors. For more details see, [sky.config.patch](https://github.com/OpenStarbound/OpenStarbound/blob/main/assets/opensb/sky.config.patch).

[Discord](https://discord.gg/f8B5bWy3bA)


## Building
Note: Some of these [texts](## "hi :3") are just tooltips rather than links. 

<details>
<summary>template sbinit.config for dist/ after build</summary>
<br>

```json
{
  "assetDirectories" : [
    "../assets/",
    "./mods/"
  ],

  "storageDirectory" : "./",
  "logDirectory" : "./logs/"
}
```

</details>
<details>
<summary><b>Windows</b></summary>
 
* Install [vcpkg](https://github.com/microsoft/vcpkg?tab=readme-ov-file#quick-start-windows) *globally*.
  * vcpkg recommends a short directory, such as `C:\src\vcpkg` or `C:\dev\vcpkg`.
  * If you're using Visual Studio, don't forget to run `vcpkg integrate install`!
* Set the **`VCPKG_ROOT`** environment value to your vcpkg dir, so that CMake can find it.
* Install [Ninja](https://ninja-build.org/ "Ninja Build System"). Either add it to your [**`PATH`**](## "Environment Value"), or just use [Scoop](https://scoop.sh/) (`scoop install ninja`)
* Check to see if your IDE has CMake support, and that it's [actually installed](## "If you're using VS, open Visual Studio Installer to install CMake.").
* Open the repo directory in your IDE - it should detect the CMake project.
* Build.
  * If you're using an IDE, it should detect the correct preset and allow you to build from within.
  * Otherwise, build manually by running CMake in the **source/** directory: `cmake --build --preset=windows-release`
* The built binaries will be in **dist/**. Copy the DLLs from **lib/windows/** and the **sbinit.config** above into **dist/** so the game can run.

</details>
<details>
<summary><b>Linux (Ubuntu)</b></summary>
 
* Make sure you're using CMake 3.23 or newer - you may need to [add Kitware's APT repo](https://apt.kitware.com/) to install a newer version.
* Install dependencies:
  * `sudo apt-get install pkg-config libxmu-dev libxi-dev libgl-dev libglu1-mesa-dev libsdl2-dev python3-jinja2 ninja-build`
* Clone [vcpkg](https://github.com/microsoft/vcpkg?tab=readme-ov-file#quick-start-unix) (outside the repo!) and bootstrap it with the linked instructions.
* Set the **`VCPKG_ROOT`** environment value to your new vcpkg directory, so that CMake can find it.
  *  `export VCPKG_ROOT=/replace/with/full/path/to/your/vcpkg/directory/`
* Change to the repo's **source/** directory, then run `cmake --build --preset=linux-release` to build.
* The built binaries will be in **dist/**. Copy the the .so libs from **lib/linux/** and the **sbinit.config** above into **dist/** so the game can run.
  * From the root dir of the repo, you can run the assembly script which is used by the GitHub Action: `scripts/ci/linux/assemble.sh`
    * This packs the game assets and copies the built binaries, premade sbinit configs & required libs into **client/** & **server/**.
 
</details>

<details>
<summary><b>Linux (Fedora)</b></summary>

Starbound in general is built from the ground up, with its own engine written in C++ on top of some basic libraries.

* CMake is a C++ build scenario generator and your first target. You need at least version 3.23. Where Ubuntu uses APT, Fedora uses DNF as package manager.

  1. `sudo dnf upgrade --refresh` to ensure your OS is up-to-date
  2. `sudo dnf install cmake`
  3. `cmake --version` to verify

* You will need at least the same dependencies ("basic libraries") as for Ubuntu. Some packages have different names or contents between Linux builds. Namely, Fedora uses "-devel" instead of "-dev" for development packages.

  1. `sudo dnf install` [pkg-config](## "will install pkgconf-pkg-config") libXmu-devel libXi-devel [libGL-devel](## "will install mesa-libGL-devel") mesa-libGLU-devel SDL2-devel python3-jinja2 ninja-build
  2. If you find out that you need any other dependencies not listed here, try finding them via [Fedora Packages](https://packages.fedoraproject.org/) first. And, preferably, improve this instruction.

* Next you will need VCPKG.

VCPKG is another package manager/dependency resolver for C++. CMake will need it to pull the rest of dependencies automatically early in the building process. If you've worked with language-specific package managers before (for example, NPM or YUM for JavaScript), VSPKG is similar. For reference, the list of dependencies VCPKG will try to install later can be found in `source/vcpkg.json`.

  1. There are many ways to get VCPKG. Here's one: `. <(curl https://aka.ms/vcpkg-init.sh -L)`. This instruction should install VCPKG in your Linux home (user profile) directory in `.vcpkg`. Note that this dir is usually hidden by default.
  2. Next you need to set your **`VCPKG_ROOT`** environment variable to the correct path. Run `. ~/.vcpkg/vcpkg-init` to bootstrap VCPKG. You may want to check if the path is now known to the system by running `printenv VCPKG_ROOT` afterwards.
  3. Step 2 (init command) should be run in **every** new Terminal (Konsole) window **before** you begin building (environment variables set in this way do not persist between terminal sessions)

* Change to the repo's **source/** directory
* *Optional.* First step for CMake is now to run VCPKG and install the remaining dependencies as per `source/vcpkg.json`. You can run this step manually via `vcpkg install` on its own to check if it works, or *skip to the next step*.

If this step throws errors, Fedora probably still lacks some packages not listed explicitly before. Read error messages to identify these packages, find them via [Fedora Packages](https://packages.fedoraproject.org/) and install with DNF. What you need most of the time is the package itself as well as its -devel and -static subpackages.
* *Optional.* Next, we can ask CMake to assemble instructions for linux build without actually running them. The instructions generated will be stored under **build/linux-release**. To do that, run `cmake --preset=linux-release` or *skip to the next step*.
* Run `cmake --build --preset=linux-release` to build. It includes previous two steps, so if any of them throw errors, you will have problems. If that's the case, run and debug them separately as described earlier, as CMake itself can just throw `Error: could not load cache` without specifying the exact problem. In case of major changes (example: you've reinstalled VCPKG to a different location and need to regenerate path to it for CMake) purge CMake cache by deleting **source/CMakeCache.txt**.

Building will take some time, be patient ;)

<details>
<summary><b>Specific problem: If your VCPKG can't build meson for libsystemd</b></summary>
<br>

Diagnosed by 

>ERROR: Value "plain" (of type "string") for combo option "Optimization level" is not one of the choices. Possible choices are (as string): "0", "g", "1", "2", "3", "s".

error in meson building logs when building libsystemd.

Fix for VCPKG is pretty fresh (May 2024) and can be found [here](https://github.com/microsoft/vcpkg/issues/37393).

</details>

* The built binaries will be in **dist/**. Copy the the .so libs from **lib/linux/** and **sbinit.config** (see beginning of this section) into **dist/** so the game can run. Sample sbinit.config can be found in **scripts/linux/**.
* From the root dir of the repo, you can run the assembly script which is used by the GitHub Action: `scripts/ci/linux/assemble.sh`. This packs the game assets and copies the built binaries, premade sbinit configs & required libs into **client_distribution/** & **server_distribution/**.

Next you need to copy original Starbound assets at **assets/packed.pak** of the Starbound copy that you own into **assets/** of either client or server dir (depending on what you're going to run).

The game now can be run by executing **client_distribution/linux/run-client.sh** (or the corresponding server bash script) from terminal.

<details>
<summary><b>Fedora-specific problem with OSS (dsp: No such audio device)</b></summary>
<br>

Diagnosed by this error message when launching *client*:

>Couldn't initialize SDL Audio: dsp: No such audio device

The reason is outlined on [StackEx](https://stackoverflow.com/questions/9248131/failed-to-open-audio-device-dev-dsp/9248166#9248166): 

> Most new Linux distributions don't provide the OSS (open sound system) compatibility layer, because access to the OSS sound device /dev/dsp was exclusive to one program at time only.

The same answer has the solution: use `padsp` to emulate dev/dsp.

* `dnf install pulseaudio-utils` to install padsp util
* execute `padsp bash run-client.sh` instead of running sh directly. To avoid doing it every time you can edit run-client.sh, replacing 

`#!/bin/sh
cd "`dirname \"$0\"`"
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./starbound "$@"`

with 

`#!/bin/sh
cd "`dirname \"$0\"`"
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" padsp ./starbound "$@"`

</details>

</details>
<details>
<summary><b>macOS</b></summary>
 
* First, you will need to have brew install. Check out how to install [Homebrew](https://brew.sh/)
* Next, install vcpkg.
 * Run ` cd ~`. This is just so that everything is local to here. 
 * Run ` git clone https://github.com/microsoft/vcpkg.git `
 * Run `cd vcpkg && ./bootstrap-vcpkg.sh`
 * Lastly, run ``` export VCPKG_ROOT=~/vcpkg
export PATH=$VCPKG_ROOT:$PATH ```
 * This last command makes vcpkg added to the current terminal path. This lasts only while the terminal is only, and will have to be rerun.
* Install cmake using `brew install cmake`
* Install ninja using `brew install ninja`
* Download the source code [here](https://github.com/OpenStarbound/OpenStarbound/archive/refs/heads/main.zip). This is the current code in main. Unpack the code to your downloads folder. 
* Unpack the zip, and open it up. Navigate to OpenStarbound-main/source using the terminal -> `cd ~/Downloads/OpenStarbound-main`. Then navigate to the source folder, using `cd source`.
<details>
 <summary>If using ARM</summary>
 * While in the source folder in your terminal, run ` cmake --preset macos-arm-release `. This will get dependencies.
 * After that command has finished, run ` cmake --build --preset macos-arm-release `. Wait for this to finish, then go to Finder. Navigate to the OpenStarbound-main folder using Finder. 
 * There will be a folder called <b>dist</b>. Inside dist will be your game files, but you still need to do a few more things to run it.
 * First, in the OpenStarbound-main folder, there will be lib. Open lib, and open the osx folder. Inside is libsteam_api.dylib. Copy this file, and paste it into OpenStarbound-main/dist, so that it is in the same directory as the game files. 
 * Navigate back to OpenStarbound-main/lib/osx, and open up the folder arm64. Here, rename libdiscord_game_sdk.dylib to discord_game_sdk.dylib. The name must be that, or else the game won't be able to load. 
 * You can now run the game. If it says unverified developer, open up the same folder where the game is in in the terminal. ` xattr -d com.apple.quarantine starbound `, which will get rid of the lock on the file. If that doesn't work, run ` sudo spctl --master-disable ` to allow all unverified apps. 
</details>
<details>
 <summary>If using Intel</summary>
 * While in the source folder in your terminal, run ` cmake --preset macos-release `. This will get dependencies.
 * After that command has finished, run ` cmake --build --preset macos-release `. Wait for this to finish, then go to Finder. Navigate to the OpenStarbound-main folder using Finder. 
 * There will be a folder called <b>dist</b>. Inside dist will be your game files, but you still need to do a few more things to run it.
 * First, in the OpenStarbound-main folder, there will be lib. Open lib, and open the osx folder. Inside is libsteam_api.dylib. Copy this file, and paste it into OpenStarbound-main/dist, so that it is in the same directory as the game files. 
 * Navigate back to OpenStarbound-main/lib/osx, and open up the folder x64. Here, rename libdiscord_game_sdk.dylib to discord_game_sdk.dylib. The name must be that, or else the game won't be able to load. 
 * You can now run the game. If it says unverified developer, open up the same folder where the game is in in the terminal. ` xattr -d com.apple.quarantine starbound `, which will get rid of the lock on the file. If that doesn't work, run ` sudo spctl --master-disable ` to allow all unverified apps. 

</details>
</details>
