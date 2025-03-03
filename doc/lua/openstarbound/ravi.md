
OpenStarbound may use Ravi for Lua, which allows JIT compilation among other things.

Warning: some functions (such as ravi.jit and ravi.auto) apply to the entire thread.  
Avoid changing the JIT/auto configuration (through ravi.auto(true) or such) unless it is on your own scriptable thread.  
Especially auto, as not all mods are meant to be JIT compiled, and auto causes lag spikes when any script contexts are created.  
Instead, use functions like ravi.compile to manually JIT compile what you want to be compiled.  

Examples:  
    ravi.compile(update) -- JIT compile one function (like the update callback)  
    ravi.compile(vec2) -- JIT compile a table of utility functions (like vec2)  
    -- Compiling a table of functions will only compile up to 100 functions at a time.  
    -- If you need to compile more, just keep calling ravi.compile on the table until ravi.compile returns true.  

Future uses of these functions should use the JIT version.

For more information, see:
https://the-ravi-programming-language.readthedocs.io/en/latest/ravi-reference.html 
(compiler.compile and compiler.loadfile are disabled when safeScripts is enabled)
