#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Chunk {
    Arr<LightLvl, CHK_VOL> light_lvl;
    Arr<BlockId , CHK_VOL> blocks;
};

extern F32 day_cycle;
extern VecSet<ChkPos> block_updated_chunks;

BlockId get_block(MapPos const& pos);
BlockBp const& get_block_bp(MapPos const& pos);

void map_tick();
void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);
Chunk& write_chunk(ChkPos const& pos);

void add_light_node(MapPos const& pos, F32 lum);
void del_light_node(MapPos const& pos);

void map_apply_suspended_updates();
