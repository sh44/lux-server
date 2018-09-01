#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
//
#include <lux/common/entity.hpp>

namespace data
{

struct Database;

struct Config
{
    Database const *db;
    EntityId player_type;
    String   server_name;
    F64      tick_rate;
};

}
