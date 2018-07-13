#pragma once

#include <alias/string.hpp>

struct EntityType;

namespace data
{

struct Database;

struct Config
{
    Database          const *db;
    EntityType const *player_type;
};

}
