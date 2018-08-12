#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/alias/string.hpp>

namespace entity { struct Type; }
namespace map    { struct TileType; }

namespace data
{

struct Database
{
    HashMap<String,  entity::Type const *> entity_types;
    HashMap<String, map::TileType const *>   tile_types;
};

}
