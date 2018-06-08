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

const world::TileType grass_tile =
{
    "grass",
    "Grass",
    world::TileType::FLOOR,
    {1, 0}
};

const data::Database default_db =
{
    {
        {human.id, &human}
    },
    {
        {void_tile.id, &void_tile}
    },
};

const data::Config default_config =
{
    &default_db,
    default_db.entity_types.at("human"),
};
