# Vendored third-party code

Everything in this directory is compiled into the binary from source. This file
records version and origin so the dependency surface is auditable in one place.

| Path | What | Version | Origin / update path |
|------|------|---------|----------------------|
| `lua/`, `lua.h`, `lualib.h`, `lauxlib.h`, `luaconf.h`, `lua.hpp` | Lua interpreter (gameplay + asset scripts) | 5.3.6 (EOL upstream; 5.4 is current) | upstream lua.org. Do NOT bump without a script-compat regression pass (`lua_test.cpp`, asset `.patch.lua` evaluation). |
| `fmt/` | Formatting (`strf`, `toString` via `StarFormat.hpp`) | 10.2.1 (`FMT_VERSION 100201`) | github.com/fmtlib/fmt; single choke point is `source/core/StarFormat.hpp` |
| `xxhash.c/.h`, `xxh3.h`, `xxh_x86dispatch.*` | Hashing | 0.8.1 | github.com/Cyan4973/xxHash |
| `fast_float.h` | Fast float parsing | versionless in-file (no macro) | github.com/fastfloat/fast_float; verify against upstream on update |
| `rpmalloc.*`, `rpnew.h` | Allocator (Windows default via `STAR_USE_RPMALLOC`) | versionless in-file | github.com/mjansson/rpmalloc |
| `curve25519/` | ed25519/curve25519 crypto (auth handshake, mod signatures) | 2016-era vendored | do not touch without security review |
| `imgui_lua_bindings.*`, `imgui_iterator.inl` | ImGui <-> Lua bridge | OpenStarbound-specific | part of this repo, not upstream |
| `malloc.c` | libc malloc replacement used by jemalloc feature | n/a | jemalloc-adjacent build shim |

`source/test/gtest/` is a vendored googletest build (compiled via
`source/test/CMakeLists.txt`), also versionless in-file — same caveat as
`fast_float.h`.

## Policy

- Prefer vcpkg for anything with an active port; keep only what vcpkg cannot
  provide or what must be patched for this project (lua 5.3 semantics, imgui
  bindings).
- When vendoring/updating, record the upstream tag/commit here in the table.
- `vcpkg.json` is pinned by `builtin-baseline`; bump it deliberately and
  rebuild CI to match.
