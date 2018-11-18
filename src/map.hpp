#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

TileId constexpr void_tile = 0;

struct Chunk {
    Arr<LightLvl, CHK_VOL> light_lvl;
    Arr<TileId  , CHK_VOL> floor;
    Arr<TileId  , CHK_VOL> wall;
    Arr<TileId  , CHK_VOL> roof;
};

extern VecSet<ChkPos> tile_updated_chunks;
extern VecSet<ChkPos> light_updated_chunks;

TileId get_floor(MapPos const& pos);
TileId get_wall(MapPos const& pos);
TileId get_roof(MapPos const& pos);
TileBp const& get_floor_bp(MapPos const& pos);
TileBp const& get_wall_bp(MapPos const& pos);
TileBp const& get_roof_bp(MapPos const& pos);

void map_tick();
void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);
Chunk& write_chunk(ChkPos const& pos);

void add_light_node(MapPos const& pos, Vec3F const& col);
void del_light_node(MapPos const& pos);
