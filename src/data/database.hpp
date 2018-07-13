#pragma once

#include <alias/hash_map.hpp>
#include <alias/string.hpp>

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
