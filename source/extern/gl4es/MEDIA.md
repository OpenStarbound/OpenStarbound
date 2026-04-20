GL4ES - OpenGL for GLES Hardware
====

Many OpenGL software already works with GL4ES.

![foobillards++](refs/foobillardplus.0000014748.png)

Complex OpenGL 1.5 games, like Foobillard++, that uses cascaded display list, line stipple and TexGen works fine. Here running on the OpenPandora. It will work with both GLES1.1 and GLES2.0 backend. Most OpenGL 1.x games will runs. 


[![Play on Youtube](https://img.youtube.com/vi/75FYb60L7zw/0.jpg)](https://www.youtube.com/watch?v=75FYb60L7zw)

The limit is an hardware one: GLES1.1 and GLES2.0 hardware do not allow reading Depth buffer, so some games that use it (like AssaultCube) for game play wont run correctly. Some FPS are using it just for cosmetics (Lens flare) and run fine without, like Serious Sam (both First and Second Encounter), here on the OpenPandora again.

![AssaultCube on Android](media/assaultcube.jpg)
AssaultCube an Android also use gl4es for the rendering


[![Play on Youtube](https://img.youtube.com/vi/kJPb2jYiBoM/0.jpg)](https://www.youtube.com/watch?v=kJPb2jYiBoM)

More complex program, like Blender, can also run.


![MineCraft](media/minecraft.png)

But also Minecraft, here on an ODroid in this old video.


![OpenRA](refs/openra.0000031249.png)

On the OpenGL 2.0, side using GLES2.0 backend, OpenRA can run on the Pandora.


And OpenGL 2.x opens a lot of doors. Many commercial games run just fine on gl4es.

[![Play on Youtube](https://img.youtube.com/vi/VUoeHWuwlMU/0.jpg)](https://www.youtube.com/watch?v=VUoeHWuwlMU)

Here some FNA games, running on an ODroid XU4 (using method described [there](https://magazine.odroid.com/article/playing-modern-fna-games-on-the-odroid-platform/)). That video show just a few, and even more can be make to run.



[![Play on Youtube](https://img.youtube.com/vi/B4YN37z3-ws/0.jpg)](https://www.youtube.com/watch?v=B4YN37z3-ws)

And with [Box86](https://github.com/ptitSeb/box86), even more commercial games can run, like here NeverWinter Night on an ODroid XU4

GL4ES also now works on some other platform, like AmigaOS4 (so BigEndian platform)
[![Play on Youtube](https://img.youtube.com/vi/hQVabA_ReoQ/0.jpg)](https://www.youtube.com/watch?v=hQVabA_ReoQ)

Here running Foobillard++

Or even in a web browser (that support WebGL): try it with Stunt Car Remake [here](http://ptitseb.github.io/stuntcarremake/)

Another web browser example: Serious Sam Engine, try it [here](https://martinmullins.github.io/ssam/) (initial 80MB game files download).
[source repo](https://github.com/martinmullins/Serious-Engine)
[![Gif of Serious Engine](https://github.com/martinmullins/ssam/blob/main/ssam.gif?raw=true)](https://martinmullins.github.io/ssam/)

Neverball in a browser also use gl4es for rendering: [neverball in a browser](https://neverball.github.io/)

About performances: while gl4es is a wrapper, there are many caching and works done in gl4es to avoid slowdown and bottleneck. While  those optimization are useless in highly optimized engine, like the Quake3 engine, on other engine and games, gl4es can be (much) faster than straight GLES conversion (especially if there are a lot of `glBegin(...)`/`glEnd()` drawing commands). On an x86 VM, gl4es running on GLES can be faster than direct OpenGL use on some games!
Now GL4ES can also use VBO, either when the software use it, or automatically when using glList, giving sometimes a nice boost (the boost depend on the Platform and game)

