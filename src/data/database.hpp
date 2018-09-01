#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/vector.hpp>
#include <lux/alias/hash_map.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/voxel.hpp>
#include <lux/common/entity.hpp>
//
#include <map/voxel_type.hpp>
#include <entity/entity_type.hpp>

namespace data
{

class Database
{
public:
    Database();

    template<typename... Args>
    void add_entity(Args const &...args);
    template<typename... Args>
    void add_voxel(Args const &...args);

    EntityType const &get_entity(String const &str_id) const;
    VoxelType  const &get_voxel(String const &str_id) const;
    EntityId   const &get_entity_id(String const &str_id) const;
    VoxelId    const &get_voxel_id(String const &str_id) const;

    Vector<EntityType> entities;
    Vector<VoxelType>  voxels;
    HashMap<String, EntityId> entities_lookup;
    HashMap<String,  VoxelId> voxels_lookup;
};

template<typename... Args>
void Database::add_entity(Args const &...args)
{
    auto const &entity = entities.emplace_back(args...);
    entities_lookup[entity.str_id] = entities.size() - 1;
}

template<typename... Args>
void Database::add_voxel(Args const &...args)
{
    auto const &vox = voxels.emplace_back(args...);
    voxels_lookup[vox.str_id] = voxels.size() - 1;
}

}
