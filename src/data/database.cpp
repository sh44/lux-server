#include <entity/type.hpp>
#include <map/tile_type.hpp>
#include "database.hpp"

namespace data
{

Database::Database()
{
    add_entity("human", "Human");
    add_tile("void", "Void", map::TileType::EMPTY);
    add_tile("stone_floor", "Stone Floor", map::TileType::WALL);
    add_tile("stone_wall", "Stone Wall", map::TileType::WALL);
    add_tile("raw_stone", "Raw Stone", map::TileType::WALL);
    add_tile("dirt", "Dirt", map::TileType::WALL);
    add_tile("gravel", "Gravel", map::TileType::WALL);
    add_tile("grass", "Grass", map::TileType::WALL);
}

entity::Type const &Database::get_entity(String const &str_id) const
{
    return entities[get_entity_id(str_id)];
}

map::TileType const &Database::get_tile(String const &str_id) const
{
    return tiles[get_tile_id(str_id)];
}

entity::Id const &Database::get_entity_id(String const &str_id) const
{
    return entities_lookup.at(str_id);
}

tile::Id const &Database::get_tile_id(String const &str_id) const
{
    return tiles_lookup.at(str_id);
}

}
