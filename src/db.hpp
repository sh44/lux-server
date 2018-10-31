#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <entity.hpp>

struct TileBp {
    DynStr str_id;
};

void db_init();
TileBp const& db_tile_bp(TileId id);
TileBp const& db_tile_bp(DynStr const& str_id);
TileId const& db_tile_id(DynStr const& str_id);
