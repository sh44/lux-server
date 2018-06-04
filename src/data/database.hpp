#pragma once

#include <alias/hash_map.hpp>
#include <alias/string.hpp>

namespace world::entity { class Type; }
namespace world::tile   { class Type; }

namespace data
{

class Database
{
    public:
    world::entity::Type const &entity_type_at(String const &id) const;
    world::tile::Type   const &tile_type_at(String const &id)   const;
    private:
    HashMap<String, world::entity::Type *> entity_types;
    HashMap<String, world::tile::Type *>   tile_types;
};

}
