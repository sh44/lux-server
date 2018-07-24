#include <entity/type.hpp>
#include <tile/type.hpp>
#include <data/database.hpp>
#include <data/config.hpp>

const entity::Type human =
{
    "human",
    "Human"
};

const tile::Type void_tile =
{
    "void",
    "Void",
    tile::Type::EMPTY
};

const tile::Type stone_floor =
{
    "stone_floor",
    "Stone Floor",
    tile::Type::FLOOR
};

const tile::Type stone_wall =
{
    "stone_wall",
    "Stone Wall",
    tile::Type::WALL
};

const data::Database default_db =
{
    {
        {human.id, &human}
    },
    {
        {void_tile.id,  &void_tile},
        {stone_floor.id, &stone_floor},
        {stone_wall.id, &stone_wall}
    },
};

const data::Config default_config =
{
    &default_db,
    default_db.entity_types.at("human"),
    "lux server",
    64.0
};
