# OP2UtilityC

A self-contained **C-ABI shared library (DLL)** wrapper around
[OP2Utility](https://github.com/OutpostUniverse/OP2Utility), the OutpostUniverse
C++ library for Outpost 2 game resources (maps, archives, graphics, …).

> Fork home: https://github.com/leviathan400/OP2UtilityC

## Why this exists

OP2Utility is a **static C++ library**: its public API is C++ classes that use
exceptions and STL types (`std::string`, `std::vector`, …). Those cannot safely
cross a DLL boundary between different compilers or runtimes — an MSVC-built DLL
can't be consumed by a MinGW/GCC app, and vice versa.

OP2UtilityC solves that by exposing a **flat C interface** instead:

- opaque handles (`Op2Map*`) — C++ objects never cross the boundary,
- plain-old-data out-parameters and integer error codes (`Op2Result`),
- **all exceptions caught inside** the library (converted to error codes + a
  message via `op2_last_error()`).

A C ABI is stable across toolchains and languages, so the resulting DLL can be
used from C, C++, C#, Python, Rust, Go — anything with FFI.

## What's wrapped

The map-loading surface, for now:

- Read a `.map` file (or the map portion of a saved game) into memory.
- Query dimensions, version tag, saved-game flag, tile count.
- Per-cell lookups: cell type, tile-mapping index, tileset index, image index,
  lava-possible.

The wrapper is intentionally easy to extend — add more `extern "C"` functions in
`capi/op2utility_c.*` to expose VOL/CLM archives, sprites, etc. as needed.

## The C API

A single header, `capi/op2utility_c.h`:

```c
#include "op2utility_c.h"

Op2Result err;
Op2Map* map = op2_map_read("eden01.map", &err);
if (!map) {
    fprintf(stderr, "load failed (%d): %s\n", err, op2_last_error());
    return 1;
}

uint32_t w = op2_map_width(map);
uint32_t h = op2_map_height(map);

int32_t cellType;
if (op2_map_cell_type(map, 15, 15, &cellType) == OP2_OK) {
    /* ... */
}

op2_map_free(map);
```

| Function | Purpose |
|---|---|
| `op2_capi_version()` | C ABI version (bump on breaking changes). |
| `op2_last_error()` | Message for the last failure (per-thread). |
| `op2_map_read(path, &err)` | Load a `.map`; returns `NULL` on failure. |
| `op2_map_read_saved_game(path, &err)` | Load the map portion of a saved game. |
| `op2_map_free(map)` | Release a loaded map. |
| `op2_map_width/height/tile_count/version_tag/is_saved_game` | Metadata. |
| `op2_map_cell_type/tile_mapping_index/tileset_index/image_index/lava_possible` | Per-cell queries. |

## Building

Requires CMake and a C++20 compiler (MSVC, GCC, or Clang — same as OP2Utility).

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Outputs (Windows/MinGW):

- `build/OP2Utility.dll` — the shared library
- `build/libOP2Utility.dll.a` — the import library to link against
- `build/op2utility_smoketest.exe` — a pure-C test that exercises the ABI

On MinGW the DLL is linked with `-static-libgcc -static-libstdc++ -static`, so it
is **self-contained** — consumers don't need a matching `libstdc++`/`libgcc` at
runtime. (Safe because only a C ABI crosses the boundary.)

## Using it in your project

1. Add `capi/op2utility_c.h` to your include path.
2. Link against the import library (`libOP2Utility.dll.a` on MinGW, `OP2Utility.lib`
   on MSVC).
3. Ship `OP2Utility.dll` alongside your executable.

## Repository layout

```
OP2UtilityC/
├── OP2Utility/          ← upstream OP2Utility sources (vendored)
├── capi/
│   ├── op2utility_c.h   ← the C ABI (the only public surface)
│   ├── op2utility_c.cpp ← wrapper implementation
│   ├── version.rc       ← Windows version-info resource
│   └── smoketest.c      ← pure-C ABI smoke test
├── CMakeLists.txt       ← builds the DLL + import lib + smoke test
└── ReadMe.md
```

## License

MIT, same as upstream. OP2Utility is © the Outpost Universe project; see
`License.txt`. The C-ABI wrapper in `capi/` is released under the same MIT terms.

## Credits

- [OP2Utility](https://github.com/OutpostUniverse/OP2Utility)

