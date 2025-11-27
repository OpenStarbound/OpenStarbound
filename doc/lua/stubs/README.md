# Lua Stub Files

This directory contains Lua stub files with EmmyLua/LuaLS annotations for OpenStarbound's Lua API.

## Purpose

These stub files provide:
- IDE autocompletion support for OpenStarbound's Lua APIs
- Type checking and parameter validation
- Function documentation directly in your editor
- Better development experience when writing mods

## Usage

### With VS Code + Lua Language Server

1. Install the [Lua](https://marketplace.visualstudio.com/items?itemName=sumneko.lua) extension
2. Add this to your workspace settings (`.vscode/settings.json`):
```json
{
  "Lua.workspace.library": [
    "/path/to/OpenStarbound/doc/lua/stubs"
  ]
}
```

### With Other IDEs

Most modern Lua IDEs support EmmyLua annotations. Refer to your IDE's documentation for how to add external library definitions.

## Contents

This directory contains stub files for:


## Contributing

If you find missing functions or incorrect type annotations, please:
1. Update the corresponding markdown documentation file in `doc/lua/openstarbound/`
2. Submit a pull request

## Notes

- Type annotations use EmmyLua/LuaLS syntax
- Optional parameters are marked with `?`
