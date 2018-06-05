#pragma once

#include <alias/string.hpp>

namespace world { struct EntityType; }

namespace data
{

struct Database;

struct Config
{
    Database          const *db;
    world::EntityType const *player_type;
};

}
