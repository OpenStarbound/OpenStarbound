Usage
----

There are many environnement variable to control gl4es behavour. All are numeric, except LIBGL_VERSION that take a string, LIBGL_FBO that takes a 2d size (WxH), and both LIBGL_EGL and LIBGL_GLES that take path/filename.
You can also change many of this variable at runtime using the `glHint(...)` function. See [gl4eshint.h](include/gl4eshint.h) for the list of #define to use in this function.

##### LIBGL_FB
Controls the Framebuffer output
 * 0 : Default, using standard x11 rendering
 * 1 : Use Framebuffer output (x11 bypassed, only fullscreen)
 * 2 : Use Framebuffer, but also an intermediary FBO
 * 3 : Use PBuffer, allowing x11 (and windowed) rendering even if driver doesn't support it 

##### LIBGL_FBONOALPHA
In case of LIBGL_FB=2, control if FBO is RGBA or RGB
 * 0 : Default, use RGBA
 * 1 : Use RGB for FBO

##### LIBGL_ES
Controls the version of GLES to use
 * 0 : Default, using GLES 2.0 backend (unless built with DEFAULT_ES 1) (not on Pandora, still GLES 1.1 backend by default)
 * 1 : Use GLES 1.1 backend
 * 2 : Use GLES 2.0 backend

##### LIBGL_GL
Controls the version of OpenGL exposed
 * 0 : Default, expose OpenGL 1.5 when using GLES1.1 or OpenGL 2.1 when using GLES2.0
 * 10..14: Export OpenGL 1.0-1.4
 * 15: Expose OpenGL 1.5 (default for GLES 1.1 backend)
 * 20: Expose OpenGL 2.0
 * 21: Expose OpenGL 2.1 (default for GLES 2.0 backend)

##### LIBGL_XREFRESH
Debug helper in specific cases
 * 0 : Default, nothing special
 * 1 : xrefresh will be called on cleanup

##### LIBGL_STACKTRACE
Automatic Backtrace log
 * 0 : Default, nothing special
 * 1 : stacktrace will be printed on crash

##### LIBGL_FPS
Print current FPS to the console
 * 0 : Defaut, don't mesure or printf FPS
 * 1 : Print FPS (on stdout) every second

##### LIBGL_VSYNC
VSync control
 * 0 : Default, nothing special
 * 1 : vsync enabled

##### LIBGL_RECYCLEFBO
Recycling FBO special case (don't delete a created FBO, but recycle it if needed)
 * 0 : Default, nothing special
 * 1 : Recycling of FBO enabled

##### LIBGL_FORCENPOT
Forcing NPOT (Non-Power of Two) Texture size.
 * 0 : Default, nothing special
 * 1 : If hardware only support Limited NPOT, then disabling MIPMAP (i.e. LIBGL_MIPMAP=3), so all texture can be NPOT.
       If Hardware support full NPOT, do nothing special. Usefull for GLES2 backend where limited NPOT is always supported.
 
##### LIBGL_TEXCOPY
Make a local copy of every texture for easy glGetTexImage2D
 * 0 : Default, nothing special
 * 1 : Texture copy enabled

##### LIBGL_SHRINK
Texture shrinking control
 * 0 : Default, nothing special
 * 1 : everything / 2 (using original algorithm for size reduction, all other shink mode use a refined algorithm)
 * 2 : only textures wich one size > 512 are / 2
 * 3 : only textures wich one size > 256 are / 2
 * 4 : only textures wich one size > 256 are / 2, and the one > 1024 are / 4
 * 5 : only textures wich one size > 256 are resized to 256 (if possible, because only /2 and /4 exists), but empty texture are not shrinked
 * 6 : only textures wich one size > 128 are / 2, thoses >= 512 are resized to 256 (if possible, because only /2 and /4 exists), but empty texture are not shrinked
 * 7 : only textures wich one size > 512 are / 2, but empty texture are not shrinked
 * 8 : advertise a max texture size *4, but every texture wich one size > 2048 are shrinked to 2048
 * 9 : advertise a max texture size *4, but every texture wich one size > 4096 are / 4 and the one > 512 are / 2, but empty texture are not shrinked
 * 10: advertise a max texture size *4, but every texture wich one size > 2048 are / 4 and the one > 512 are / 2, but empty texture are not shrinked
 * 11: advertise a max texture size *2, but every texture with one dimension > max texture size will get shrinked to max texture size
 
##### LIBGL_TEXDUMP
Texture dump
 * 0 : Default, nothing special
 * 1 : Texture dump enabled

##### LIBGL_ALPHAHACK
Experimental: enable Alpha test only when using texture that contains an alpha channel
 * 0 : Default, nothing special
 * 1 : Alpha Hack enabled

##### LIBGL_NODOWNSAMPLING
Texture downsampling control (deprecated, use LIBGL_AVOID16BITS instead)
 * 0 : Default, DXTc texture are downsampled to 16bits
 * 1 : DXTc texture are left as 32bits RGBA

##### LIBGL_STREAM
PANDORA only: enable Texture Streaming (works only on RGB textures)
 * 0 : Default, nothing special
 * 1 : Enabled on empty RGB textures
 * 2 : Enabled on all RGB textures

##### LIBGL_COPY
Removed (Controled the glCopyTex(Sub)Image2D hack, it's now automatic, depending on how compatible is the readed framebuffer)

##### LIBGL_NOLUMALPHA
Control the availability of the LUMUNANCE_ALPHA format (can be buggy on Pandora model CC)
 * 0 : Default,GL_LUMINANCE_ALPHA is available and used if needed
 * 1 : GL_LUMINANCE_ALPHA hardware support disabled (a GL_RGBA texture will be used instead)

##### LIBGL_BLENDHACK
Experimental: Change Blend GL_SRC_ALPHA, GL_ONE to GL_ONE, GL_ONE
 * 0 : Default, nothing special
 * 1 : Change Blend GL_SRC_ALPHA, GL_ONE to GL_ONE, GL_ONE (can be usefull for Xash3D engine)

##### LIBGL_BLENDCOLOR
Hack: Export a (faked) glBlendColor
 * 0 : Default, don't expose gBlendColor
 * 1 : Exposed the function (if no hadware support, faked function willbe used)

##### LIBGL_VERSION
Hack: Control the glGetString version. Overide version string (should be in the form of "1.x")

##### LIBGL_BATCH
This has been changed with v1.0.5.
Now BATCH simply try to merge subsequents glDrawXXXXX (glDrawArrays, glDrawElements...). It only try to merge if arrays is between MINBATCH and MAXBATCH (inclusive)
The Batching stop when there is a change of GL State, but also if an Array of more then 100*N is encountered.
 * 0 : Default: don't try to merge glDrawXXXXX
 * N : Any number: try to merger arrays, 1st must be between 0 and 100*N
 * MIN-MAX : 2 number separated by minus, to try merge arrays that are between MIN and MAX vertices

##### LIBGL_NOERROR
Hack: glGetError() always return GL_NOERROR
 * 0 : Default, glGetError behave as it should
 * 1 : glGetError never fail.

##### LIBGL_GAMMA
Pandora Hack: Set a Gamma value (in decimal formal, 1.0 means no gamma boost)
 * X.Y : Use X.Y as gamma when creating context (typical value can be 1.6 or 2.0)

##### LIBGL_SRGB
ODROID Hack: Enable sRGB Surface (so Gamma corrected), if Hardware support it
 * 0 : Default, don't try to use sRGB surface
 * 1 : Enable sRGB Surface (but support will be tested first, must have EGL_KHR_gl_colorspace extension)

##### LIBGL_FASTMATH
Hack: Activate some Fast Math in processor/coprocessor
 * 0 : Default, nothing special
 * 1 : On OpenPandora and CHIP, activate "RunFast" on Cortex-A8 (mode default NaN, flush-to-zero)
     : Not implemented on other platforms (will do nothing)

##### LIBGL_SILENTSTUB
Debug: Hide or Show the Sub / Not found message
 * 0 : The messages for Stub or absend function are printed
 * 1 : Default, don't print the STUB or glXGetProcAddress glXXXXX not found message

##### LIBGL_NOBANNER
Show/Hide initial text
 * 0 : Default, print starting message
 * 1 : Silent: no LIBGL message at start (combine with LIBGL_SILENTSTUB for more silence)

##### LIBGL_NPOT
Expose NPOT (Non Power of Two) Support
 * 0 : Default, expose the extension that are availble by the GLES backend
 * 1 : Expose limited NPOT extension
 * 2 : Expose GL_ARB_texture_non_power_of_two extension

##### LIBGL_GLQUERIES
Expose glQueries functions
 * 0 : Don't expose the function (fake one will be used if called)
 * 1 : Default, expose fake functions (always answer 0)

##### LIBGL_NOTEXMAT
Handling of Texture Matrix
 * 0 : Default, perform handling internaly (better handling of NPOT texture on all hardware)
 * 1 : Let the driver handle texmat (can be faster in some cases, but NPOT texture may be broken)

##### LIBGL_NOTEST
Initial Hardware test
 * 0 : Default, perform intial hardware testing (using a PBuffer)
 * 1 : Do not perform test (no extensions tested or used)

##### LIBGL_NOVAOCACHE
VAO Caching
 * 0 : Default, try to cache vao to avoid memcpy in render list
 * 1 : Don't cache VAO

##### LIBGL_VABGRA
Vertex Array BGRA extension
 * 0 : Default, GL_ARB_vertex_array_bgra not exposed (still emulated)
 * 1 : Extension exposed may be faster in some cases (Arx Libertatis mainly)

##### LIBGL_BEGINEND
Merge of subsequent glBegin/glEnd blocks (will be non-effective if BATCH mode is used)
 * 0 : Don't try to merge
 * 1 : Try to merge, even if there is a glColor / glNormal in between (default)

##### LIBGL_AVOID16BITS
Try to avoid 16bits textures
 * 0 : Default on ImgTec hardware, use 16bits texture if it can avoid a convertion or for DXTc textures
 * 1 : Default on all other hardware, Use 32bits texture unless specifically requested (using internalformat)

##### LIBGL_AVOID24BITS
Try to avoid 24bits textures (i.e. GL_RGB)
 * 0 : Default, use 24bits texture when it's possible
 * 1 : Force 32bits textures when GL_RGB is asked (as internal or not). Not recommanded, as it may break some blend functions (especialy on GLES 1.1 backend). Does not impact 16bits formats.

##### LIBGL_FORCE16BITS
Try to use 16bits textures
 * 0 : Default, don't force 16bits texture
 * 1 : Use 16bits texture instead of 32bits (i.e. use RGBA4 instead of RGBA8 and RGB5 instead of RGB8)

##### LIBGL_POTFRAMEBUFFER
Use only Power Of Two dimension for Framebuffer
 * 0 : Default, use NPOT dimension if supported
 * 1 : Force Framebuffer to be created with POT dimension (not advised on GLES2 backend)

##### LIBGL_NOBGRA
Ignore BGRA Texture hardware extension
 * 0 : Default, use BGRA extension if possible
 * 1 : Ignore BGRA extension, even if supported by GLES hardware

##### LIBGL_NOTEXRECT
Don't expose Texture rectangle extension (GL_ARB_texture_rectangle)
 * 0 : Default, the extension is listed
 * 1 : Don't expose the extension (it's not supported in shaders yet)

##### LIBGL_NOHIGHP
Usage of highp precision in fragment shader (ES2 backend only)
 * 0 : Default, use highp if available
 * 1 : Disable usage of highp in Fragment shaders

##### LIBGL_COMMENTS
Comments in shaders are kept (also for generated shaders by fpe_shaders)
 * 0 : Default, no comments in shaders sent to GLES Hardware
 * 1 : Comments are left in Shaders sent to GLES Hardware

##### LIBGL_DEFAULTWRAP
Hack to define default WRAP mode for texture
* 0 : Default wrap mode is GL_REPEAT (normal OpenGL behavour): default on NPOT hardware
* 1 : Default wrap mode is GL_CLAMP_TO_EDGE: default on limited NPOT or non-NPOT hardware
* 2 : Default wrap mode is GL_CLAMP_TO_EDGE, enforced (not advised)

##### LIBL_FBOUNBIND
Workaround on FBO where a binded texture is used for drawing
* 0 : Disabled (Default for all other configuration)
* 1 : Enabled (Default on ARM and PowerVR hardware)

##### LIBGL_FBOFORCETEX
For the Color Attachment 0 to be a Texture2D (even if program attachs a Renderbuffer) => may speedup glBlitFramebuffer if used
* 0 : Default, don't force
* 1 : For Color Attachment 0 of FBO to be a texture

##### LIBGL_FBO
Hack: define custom dimension for FBO (only used with LIBGL_FBO=2)
* WxH : Define FBO of WxH size (ex: LIBGL_FBO=1280x720)

##### LIBGL_NOTEXARRAY
Hack to force using discrete Texture instead of Array in all shader
* 0 : Default: Array of texture is used in shaders (not in FPE generated ones)
* 1 : Individual texture are forced in shaders (shaders may fail to compile if array are accessed by indice)

##### LIBGL_LOGSHADERERROR
Log to the console Shader Compile error, with initial and ShaderConv'd source of the shader
* 0 : Default, don't log
* 1 : Log Shader Compilation Errors

##### LIBGL_SHADERNOGLES
Don't use GL_ES part in shaders
* 0 : Default, let GL_ES part in shader
* 1 : Remove the GL_ES part in shader (usefull for Löve for example)

##### LIBGL_NODEPTHTEX
Disable the use of Depth texture
* 0 : Default, Use Depth Texture if supported by Hardware
* 1 : Disable the use of Depth Texture (renderbuffer will be used in FBO)

##### LIBGL_FLOAT
Expose support for FLOAT and HALF_FLOAT Texture support (and has attachement to FBO)
* 0 : Don't exposed, even if supported in hardware
* 1 : Default, exposed what is supported by hardware
* 2 : Force exposed, even if no supported (will be emulated has GL_UNSIGNED_BYTE if not supported)

##### LIBGL_GLXRECYCLE
Recycle EGLSurface per Drawable, instead of destroying them
* 0 : Default, don't recycle
* 1 : Don't destroy EGLSurface, per reused them per drawable (can fix EGL_BAD_ALLOC error). EGLSurface are never destroyed in this mode for now.

##### LIBGL_NOCLEAN
Debug: don't clean GLContext when they are destroy
* 0 : Default, clean GLContext 
* 1 : Don't clean GLContext

##### LIBGL_EGL
Define EGL lib to use. Default folder are the standard one for dynamic librarie loading (LD_LIBRARY_PATH and friend) plus "/opt/vc/lib/", /usr/local/lib/" and "/usr/lib/".
* by default try to use libbrcmEGL and libEGL
* filename: try to load from the defaults folder (don't forget to use complete filename, with ".so" extension). If not found/loaded, default one will be tried.
* /path/to/filename: try to use exact path/filename. If not found/loaded, default one will be tried.

##### LIBGL_GLES
Define GLES(2) lib to use. Default folder are the standard one for dynamic librarie loading (LD_LIBRARY_PATH and friend) plus "/opt/vc/lib/", /usr/local/lib/" and "/usr/lib/". Be sure to point to correct GLES library depanding on wich GLES backend you are using.
* by default try to use libGLESv1_CM, libGLES_CM or libbrcmGLESv1_CM for GLES1.1 and libGLESv2_CM, libGLESv2 or libbrcmGLESv2 for GLES2 backend
* filename: try to load from the defaults folder (don't forget to use complete filename, with ".so" extension). If not found/loaded, default one will be tried.
* /path/to/filename: try to use exact path/filename. If not found/loaded, default one will be tried.

##### LIBGL_DBGSHADERCONV
Log to the console all shaders before and after conversion
* 0 : Default: don't log anything
* 1 : Log Vertex Shader
* 2 : Log Framegent Shader
* 4 : Log Shaders before going to Shaderconv
* 8 : Log Shaders after going to Shaderconv
Note that you can combine (logical or state. So 14 will be only Fragment shader before and after shaderconv)
Note also that if neither Fragment and Vertex are defined, both will be selected. Same for Before and After.
At last, the value "1" will be changed to "15", to log everything.

##### LIBGL_VGPU_DUMP
Log to the console all shaders before and after passing through the VGPU shader conversion pipeline
* 0 : Default, don't log anything
* 1 : Log every shader going though the pipeline

##### LIBGL_VGPU_FORCE
Force the vgpu shader conversion pipeline to convert every shader.
* 0 : Default, don't convert #version 100 shaders
* 1 : Convert every shader, regardless of the #version directive

##### LIBGL_VGPU_PRECISION
Override the precision used on the pipeline
* 0 : Default, no changes are made
* 1 : Force highp where possible
* 2 : Force mediump where possible
* 3 : Force lowp where possible
Many drivers map lowp to the same precision as mediump

##### LIBGL_VGPU_BACKPORT
Whether the shader gets forcefully backported to the #version 100
* 0 : Default, don't attempt to forcefully backport the shader
* 1 : Try to forcefully backport the shader. The resulting shader might bug but it compiles.

##### LIBGL_NOPSA
Disable the use of the Precompiled Shader Archive
* 0 : Default: use (and save) the PSA (it's saved on $HOME/.gl4es.psa on linux)
* 1 : Don't use PSA.

##### LIBGL_USEVBO
Usage of VBO in certain cases. Only for GLES2+. The 2 and 3 mode are experimental and will probably be slower anyway.
* 0 : Disable the use of VBO.
* 1 : Default: Use VBO when possible (for Arrays VBO or glList)
* 2 : Use VBO when possible (and also on `glLockArrays`).
* 3 : Use VBO when possible (and special case on `glLockArrays` for idTech3 engine games).

##### LIBGL_NOES2COMPAT
Don't expose GLX_EXT_create_context_es2_profile extension
* 0 : Extension is there
* 1 : Don't expose the extension: for SDL2, use it with SDL_VIDEO_GL_DRIVER and SDL_VIDEO_EGL_DRIVER to use GLESv2 driver directly

##### LIBGL_NOINTOVLHACK
Disable the hack in shader converter to define overloaded function with int
* 0 : Default: use the hack
* 1 : Don't use it (some driver / PVRCapture don't like it much).

##### LIBGL_GLXNATIVE
Disable the filtering of GLXConfig by NATIVE_TYPE
* 0 : Default, GLX_X_NATIVE_TYPE attribute are taken into account
* 1 : Don't filter GLXConfig by GLX_X_NATIVE_TYPE

##### LIBGL_NOSHADERLOD
Disable GL_EXT_shader_texture_lod
* 0 : Default, use the extension if present
* 1 : Disable the use of the extension (using crude fallback)
