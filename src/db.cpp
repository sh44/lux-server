#include <functional>
//
#include <lux_shared/common.hpp>
//
#include "db.hpp"

static DynArr<TileBp> tiles;
//@CONSIDER use regular map?
static HashMap<Str, TileId> tiles_lookup;

void add_tile(TileBp &&tile_type) {
    auto &tile = tiles.push(std::move(tile_type));
    tiles_lookup[tile.str_id] = tiles.len - 1;
    LUX_LOG("added tile %zu: \"%.*s\"",
            tiles.len - 1, (int)tile.str_id.len, tile.str_id.beg);
}

void db_init() {
    add_tile({"void"_l});
    add_tile({"stone_floor"_l});
    add_tile({"stone_wall"_l});
    add_tile({"raw_stone"_l});
    add_tile({"dirt"_l});
    add_tile({"grass"_l});
    add_tile({"dark_grass"_l});
    add_tile({"tree_trunk"_l});
    add_tile({"tree_leaves"_l});
}

TileBp const& db_tile_bp(TileId id) {
    LUX_ASSERT(id < tiles.len);
    return tiles[id];
}

TileBp const& db_tile_bp(Str const& str_id) {
    return db_tile_bp(db_tile_id(str_id));
}

TileId const& db_tile_id(Str const& str_id) {
    LUX_ASSERT(tiles_lookup.count(str_id) > 0);
    return tiles_lookup.at(str_id);
}
