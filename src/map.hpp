#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Chunk {
    Arr<LightLvl, CHK_VOL> light_lvl;
    Arr<TileId  , CHK_VOL> bg_id;
    Arr<TileId  , CHK_VOL> fg_id;
    BitArr<CHK_VOL>         wall;
};

TileId get_fg_id(MapPos const& pos);
TileId get_bg_id(MapPos const& pos);
TileBp const& get_bg_bp(MapPos const& pos);
TileBp const& get_fg_bp(MapPos const& pos);
bool is_tile_wall(MapPos const& pos);

void map_tick(DynArr<ChkPos>& light_updated_chunks);
void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);

void add_light_node(MapPos const& pos, Vec3F const& col);
void del_light_node(MapPos const& pos);
