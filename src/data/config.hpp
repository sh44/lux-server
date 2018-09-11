#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common.hpp>
#include <lux/world/entity.hpp>

namespace data
{

struct Database;

struct Config
{
    LUX_NO_COPY(Config);
    Database const *db;
    EntityId player_type;
    String   server_name;
    F64      tick_rate;
};

}
