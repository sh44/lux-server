#pragma once

#include <lux_shared/map.hpp>

struct Chunk {
    Arr<VoxelId , CHK_VOL> voxels;
    Arr<LightLvl, CHK_VOL> light_lvls;
};

void guarantee_chunk(ChkPos const& pos);
Chunk const& get_chunk(ChkPos const& pos);