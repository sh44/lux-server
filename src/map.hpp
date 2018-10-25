#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Chunk {
    Arr<VoxelId , CHK_VOL> voxels;
    Arr<LightLvl, CHK_VOL> light_lvls;
};

void map_tick(DynArr<ChkPos>& light_updated_chunks);
void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);
VoxelId   get_voxel(MapPos const& pos);
VoxelType const& get_voxel_type(MapPos const& pos);

void add_light_node(MapPos const& pos, Vec3F const& col);
void del_light_node(MapPos const& pos);
