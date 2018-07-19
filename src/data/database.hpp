#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/alias/string.hpp>

namespace entity { struct Type; }
namespace tile   { struct Type; }

namespace data
{

struct Database
{
    HashMap<String, entity::Type const *> entity_types;
    HashMap<String,   tile::Type const *>   tile_types;
};

}
