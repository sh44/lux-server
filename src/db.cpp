#include <functional>
//
#include <lux_shared/common.hpp>
//
#include "db.hpp"

static DynArr<TileBp> tiles;
//@CONSIDER use regular map?
static HashMap<DynStr, TileId> tiles_lookup;

void add_tile(TileBp &&tile_type) {
    auto &tile = tiles.emplace_back(tile_type);
    tiles_lookup[tile.str_id] = tiles.size() - 1;
}

void db_init() {
    add_tile({"void"});
    add_tile({"stone_floor"});
    add_tile({"stone_wall"});
    add_tile({"raw_stone"});
    add_tile({"dirt"});
    add_tile({"grass"});
    add_tile({"dark_grass"});
}

TileBp const& db_tile_bp(TileId id) {
    LUX_ASSERT(id < tiles.size());
    return tiles[id];
}

TileBp const& db_tile_bp(DynStr const& str_id) {
    return db_tile_bp(db_tile_id(str_id));
}

TileId const& db_tile_id(DynStr const& str_id) {
    LUX_ASSERT(tiles_lookup.count(str_id) > 0);
    return tiles_lookup.at(str_id);
}
