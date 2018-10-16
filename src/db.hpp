#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <entity.hpp>

struct VoxelType {
    DynStr str_id;
    enum Shape {
        EMPTY,
        FLOOR,
        BLOCK,
        HALF_BLOCK,
    } shape;
};

void db_init();
VoxelType const& db_voxel_type(VoxelId id);
VoxelType const& db_voxel_type(DynStr const& str_id);
VoxelId   const& db_voxel_id(DynStr const& str_id);
