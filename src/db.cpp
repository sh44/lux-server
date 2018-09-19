#include <functional>
#include "db.hpp"

static DynArr<VoxelType> voxels;
static HashMap<String, VoxelId, std::hash<String>> voxels_lookup;

void add_voxel(VoxelType &&voxel_type) {
    auto &voxel = voxels.emplace_back(voxel_type);
    voxels_lookup[voxel.str_id] = voxels.size() - 1;
}

void db_init() {
    add_voxel({"void", VoxelType::EMPTY});
    add_voxel({"stone_floor", VoxelType::FLOOR});
    add_voxel({"stone_wall", VoxelType::BLOCK});
    add_voxel({"raw_stone", VoxelType::BLOCK});
    add_voxel({"dirt", VoxelType::BLOCK});
    add_voxel({"gravel", VoxelType::BLOCK});
}

VoxelType const& db_voxel_type(VoxelId id) {
    LUX_ASSERT(id < voxels.size());
    return voxels[id];
}

VoxelType const& db_voxel_type(String const& str_id) {
    return db_voxel_type(db_voxel_id(str_id));
}

VoxelId const& db_voxel_id(String const& str_id) {
    LUX_ASSERT(voxels_lookup.count(str_id) > 0);
    return voxels_lookup.at(str_id);
}
