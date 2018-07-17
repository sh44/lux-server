#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/alias/string.hpp>

struct EntityType;
struct TileType;

namespace data
{

struct Database
{
    HashMap<String, EntityType const *> entity_types;
    HashMap<String, TileType   const *>   tile_types;
};

}
