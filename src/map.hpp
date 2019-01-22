#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Block {
    BlockId  id;
    BlockLvl lvl;
};
struct Chunk {
    Arr<Block   , CHK_VOL> blocks;
    Arr<LightLvl, CHK_VOL> light_lvl;
    IdSet<ChkIdx> updated_blocks;
};

extern F32 day_cycle;
extern VecSet<ChkPos> updated_chunks;

Block get_block(MapPos const& pos);
BlockBp const& get_block_bp(MapPos const& pos);

void map_tick();
void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);
Block& write_block(MapPos const& pos);

void add_light_node(MapPos const& pos, F32 lum);
void del_light_node(MapPos const& pos);

void map_apply_suspended_updates();
bool map_cast_ray_exterior(MapPos *out, Vec3F src, Vec3F dst);
bool map_cast_ray_interior(MapPos *out, Vec3F src, Vec3F dst);
