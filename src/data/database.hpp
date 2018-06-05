#pragma once

#include <alias/hash_map.hpp>
#include <alias/string.hpp>

namespace world
{
    struct EntityType;
    struct TileType;
}

namespace data
{

struct Database
{
    HashMap<String, world::EntityType const *> entity_types;
    HashMap<String, world::TileType   const *>   tile_types;
};

}
