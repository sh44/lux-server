#pragma once

#include <alias/string.hpp>

namespace world { class EntityType; }

namespace data
{

class Database;

struct Config
{
    Database          const *db;
    world::EntityType const *player_type;
};

}
