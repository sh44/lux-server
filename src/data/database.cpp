#include <entity/entity_type.hpp>
#include <map/voxel_type.hpp>
#include "database.hpp"

namespace data
{

Database::Database()
{
    add_entity("human", "Human");
    add_voxel("void", "Void", VoxelType::EMPTY);
    add_voxel("stone_floor", "Stone Floor", VoxelType::WALL);
    add_voxel("stone_wall", "Stone Wall", VoxelType::WALL);
    add_voxel("raw_stone", "Raw Stone", VoxelType::WALL);
    add_voxel("dirt", "Dirt", VoxelType::WALL);
    add_voxel("gravel", "Gravel", VoxelType::WALL);
    add_voxel("grass", "Grass", VoxelType::WALL);
}

EntityType const &Database::get_entity(String const &str_id) const
{
    return entities[get_entity_id(str_id)];
}

VoxelType const &Database::get_voxel(String const &str_id) const
{
    return voxels[get_voxel_id(str_id)];
}

EntityId const &Database::get_entity_id(String const &str_id) const
{
    return entities_lookup.at(str_id);
}

VoxelId const &Database::get_voxel_id(String const &str_id) const
{
    return voxels_lookup.at(str_id);
}

}
