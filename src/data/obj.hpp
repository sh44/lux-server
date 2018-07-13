#include <entity/entity_type.hpp>
#include <tile/tile_type.hpp>
#include <data/database.hpp>
#include <data/config.hpp>

const EntityType human =
{
    "human",
    "Human"
};

const TileType void_tile =
{
    "void",
    "Void",
    TileType::EMPTY,
    {0, 0}
};

const TileType stone_floor =
{
    "stone_floor",
    "Stone Floor",
    TileType::FLOOR,
    {1, 0}
};

const TileType stone_wall =
{
    "stone_wall",
    "Stone Wall",
    TileType::WALL,
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
