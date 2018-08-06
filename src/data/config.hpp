#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>

namespace entity { struct Type; }

namespace data
{

struct Database;

struct Config
{
    Database     const *db;
    entity::Type const *player_type;
    String              server_name;
    F64                 tick_rate;
};

}
