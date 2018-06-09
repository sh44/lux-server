#include <world/entity/type.hpp>
#include <world/tile/type.hpp>
#include <data/database.hpp>
#include <data/config.hpp>

const world::EntityType human =
{
    "human",
    "Human"
};

const world::TileType void_tile =
{
    "void",
    "Void",
    world::TileType::EMPTY,
    {0, 0}
};

const world::TileType stone_floor =
{
    "stone_floor",
    "Stone Floor",
    world::TileType::FLOOR,
    {1, 0}
};

const world::TileType stone_wall =
{
    "stone_wall",
    "Stone Wall",
    world::TileType::WALL,
    {2, 0}
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
};
