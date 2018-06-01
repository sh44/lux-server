#pragma once

#include <alias/hash_map.hpp>
#include <alias/string.hpp>

namespace world::entity { class Type; }
namespace world::tile   { class Type; }

namespace data
{

class LuaEngine;

class GeneratorData
{
    public:
    GeneratorData(LuaEngine &lua_engine);

    world::entity::Type &entity_type_at(String const &id);
    world::tile::Type   &tile_type_at(String const &id);
    private:
    HashMap<String, world::entity::Type *> entity_types;
    HashMap<String, world::tile::Type *>   tile_types;
};

}
