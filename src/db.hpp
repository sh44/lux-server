#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <entity.hpp>

struct TileBp {
    TileBp(Str str) :
        str_id(str) { }
    StrBuff str_id;
};

void db_init();
TileBp const& db_tile_bp(TileId id);
TileBp const& db_tile_bp(Str const& str_id);
TileId const& db_tile_id(Str const& str_id);
