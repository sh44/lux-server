#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/vector.hpp>
#include <lux/alias/hash_map.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/tile.hpp>
#include <lux/common/entity.hpp>

namespace entity { struct Type; }
namespace map    { struct TileType; }

namespace data
{

class Database
{
public:
    Database();

    template<typename... Args>
    void add_entity(Args const &...args);
    template<typename... Args>
    void add_tile(Args const &...args);

    entity::Type  const &get_entity(String const &str_id) const;
    map::TileType const &get_tile(String const &str_id) const;
    entity::Id const &get_entity_id(String const &str_id) const;
    tile::Id   const &get_tile_id(String const &str_id) const;
private:
    Vector< entity::Type> entities;
    Vector<map::TileType> tiles;
    HashMap<String, entity::Id> entities_lookup;
    HashMap<String,   tile::Id> tiles_lookup;
};

template<typename... Args>
void Database::add_entity(Args const &...args)
{
    auto &entity = entities.emplace_back(args...);
    entity.id = entities.size() - 1;
    entities_lookup[entity.str_id] = entity.id;
}

template<typename... Args>
void Database::add_tile(Args const &...args)
{
    auto &tile = tiles.emplace_back(args...);
    tile.id = tiles.size() - 1;
    tiles_lookup[tile.str_id] = tile.id;
}

}
