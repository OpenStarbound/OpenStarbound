---@meta

--- thread API
---@class thread
thread = {}

--- Stops the thread. This does not destroy the thread; the parent script still has to stop the thread itself to destroy it, so avoid using this too much as it can cause memory leaks.
---@return void
function thread.stop() end

--- threads API
---@class threads
threads = {}

--- Creates a thread using the given parameters, and returns the thread's name. Here's an example that uses all available parameters: ```lua threads.create({ name="example", -- this is the thread name you'll use to index the thread scripts={ main={"/scripts/examplethread.lua"}, -- a list of scripts for each context, similarly to how other scripts work other={"/scripts/examplesecondthreadscript.lua"}, -- threads can have multiple contexts }, instructionLimit=100000000, -- optional, threads are allowed to change their own instruction limit (as they have nothing else to block if stuck) tickRate=60, -- optional, how many ticks per second the thread runs at, defaults to 60 but can be any number updateMeasureWindow=0.5, -- optional, defaults to 0.5, changing this is unnecessary unless you really care about an accurate tickrate for some reason someParameter="scungus" -- parameters for the scripts, all parameters are accessible using config.getParameter in the scripts }), ``` ---
---@param parameters Json
---@return string
function threads.create(parameters) end

--- Pauses or unpauses a thread. ---
---@param name string
---@param paused boolean
---@return void
function threads.setPause(name, paused) end

--- Stops and destroys a thread. ---
---@param name string
---@return void
function threads.stop(name) end

--- Sends a message to the given thread. Note that the return value from this is currently the only way to get data from the thread. --- Threads have simple updateable scripts with access to only a few tables. They include: - the basic tables all scripts have access to (including `threads`) - `updateablescript` bindings - `message` - `config` - `thread` --- The `thread` table has only a single function.
---@param threadName string
---@param messageName string
---@param args LuaValue
---@return RpcPromise<Json>
function threads.sendMessage(threadName, messageName, args) end
