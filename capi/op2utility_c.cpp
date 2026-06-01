// Implementation of the C ABI wrapper around OP2Utility's Map class.
// Every entry point catches all exceptions and converts them to error codes —
// nothing C++ ever crosses the DLL boundary.

#ifndef OP2UTILITYC_BUILD
#define OP2UTILITYC_BUILD
#endif
#include "op2utility_c.h"

#include <new>
#include <string>

#include "Map/Map.h"
#include "Map/CellType.h"

namespace {

// Per-thread last-error message (so op2_last_error is thread-safe).
thread_local std::string g_lastError;

void setError(const char* msg) { g_lastError = msg ? msg : ""; }
void setError(const std::string& msg) { g_lastError = msg; }

} // namespace

// Opaque handle definition: owns one OP2Utility::Map.
struct Op2Map {
    OP2Utility::Map map;
};

namespace {
// Shared bounds check + exception guard for per-cell queries. (C++ helper —
// must live OUTSIDE the extern "C" block: templates can't have C linkage.)
template <typename Fn>
Op2Result cellQuery(const Op2Map* map, uint32_t x, uint32_t y, Fn&& fn) {
    if (!map) { setError("null map"); return OP2_ERR_NULL; }
    if (x >= map->map.WidthInTiles() || y >= map->map.HeightInTiles()) {
        setError("cell out of bounds");
        return OP2_ERR_BOUNDS;
    }
    try {
        fn(map->map, x, y);
        g_lastError.clear();
        return OP2_OK;
    } catch (const std::exception& e) {
        setError(e.what());
        return OP2_ERR_UNKNOWN;
    } catch (...) {
        setError("unknown error");
        return OP2_ERR_UNKNOWN;
    }
}
} // namespace

extern "C" {

uint32_t op2_capi_version(void) { return 1; }

const char* op2_last_error(void) { return g_lastError.c_str(); }

static Op2Map* readImpl(const char* filename, Op2Result* outErr, bool savedGame) {
    auto setOut = [&](Op2Result r) { if (outErr) *outErr = r; };
    if (!filename) {
        setError("filename is null");
        setOut(OP2_ERR_NULL);
        return nullptr;
    }
    try {
        auto* handle = new Op2Map();
        handle->map = savedGame ? OP2Utility::Map::ReadSavedGame(std::string(filename))
                                : OP2Utility::Map::ReadMap(std::string(filename));
        g_lastError.clear();
        setOut(OP2_OK);
        return handle;
    } catch (const std::bad_alloc&) {
        setError("out of memory");
        setOut(OP2_ERR_UNKNOWN);
    } catch (const std::exception& e) {
        setError(e.what());
        // Heuristic: distinguish "can't open" from "parse" where possible.
        setOut(OP2_ERR_PARSE);
    } catch (...) {
        setError("unknown error");
        setOut(OP2_ERR_UNKNOWN);
    }
    return nullptr;
}

Op2Map* op2_map_read(const char* filename, Op2Result* outErr) {
    return readImpl(filename, outErr, /*savedGame*/ false);
}

Op2Map* op2_map_read_saved_game(const char* filename, Op2Result* outErr) {
    return readImpl(filename, outErr, /*savedGame*/ true);
}

void op2_map_free(Op2Map* map) { delete map; }

uint32_t op2_map_width(const Op2Map* map) {
    return map ? map->map.WidthInTiles() : 0;
}

uint32_t op2_map_height(const Op2Map* map) {
    return map ? map->map.HeightInTiles() : 0;
}

uint64_t op2_map_tile_count(const Op2Map* map) {
    return map ? static_cast<uint64_t>(map->map.TileCount()) : 0;
}

uint32_t op2_map_version_tag(const Op2Map* map) {
    return map ? map->map.GetVersionTag() : 0;
}

int32_t op2_map_is_saved_game(const Op2Map* map) {
    return map ? (map->map.IsSavedGame() ? 1 : 0) : -1;
}

Op2Result op2_map_cell_type(const Op2Map* map, uint32_t x, uint32_t y, int32_t* out) {
    if (!out) { setError("out is null"); return OP2_ERR_NULL; }
    return cellQuery(map, x, y, [&](const OP2Utility::Map& m, uint32_t cx, uint32_t cy) {
        *out = static_cast<int32_t>(m.GetCellType(cx, cy));
    });
}

Op2Result op2_map_tile_mapping_index(const Op2Map* map, uint32_t x, uint32_t y, uint32_t* out) {
    if (!out) { setError("out is null"); return OP2_ERR_NULL; }
    return cellQuery(map, x, y, [&](const OP2Utility::Map& m, uint32_t cx, uint32_t cy) {
        *out = static_cast<uint32_t>(m.GetTileMappingIndex(cx, cy));
    });
}

Op2Result op2_map_tileset_index(const Op2Map* map, uint32_t x, uint32_t y, uint32_t* out) {
    if (!out) { setError("out is null"); return OP2_ERR_NULL; }
    return cellQuery(map, x, y, [&](const OP2Utility::Map& m, uint32_t cx, uint32_t cy) {
        *out = static_cast<uint32_t>(m.GetTilesetIndex(cx, cy));
    });
}

Op2Result op2_map_image_index(const Op2Map* map, uint32_t x, uint32_t y, uint32_t* out) {
    if (!out) { setError("out is null"); return OP2_ERR_NULL; }
    return cellQuery(map, x, y, [&](const OP2Utility::Map& m, uint32_t cx, uint32_t cy) {
        *out = static_cast<uint32_t>(m.GetImageIndex(cx, cy));
    });
}

Op2Result op2_map_lava_possible(const Op2Map* map, uint32_t x, uint32_t y, int32_t* out) {
    if (!out) { setError("out is null"); return OP2_ERR_NULL; }
    return cellQuery(map, x, y, [&](const OP2Utility::Map& m, uint32_t cx, uint32_t cy) {
        *out = m.GetLavaPossible(cx, cy) ? 1 : 0;
    });
}

} // extern "C"
