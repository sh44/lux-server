#pragma once

#include <alias/string.hpp>

namespace world::entity { class Type; }

namespace data
{

class Database;
class LuaEngine;

struct Config
{
    Database            *db;
    world::entity::Type *player_type;
};

Config load_config(LuaEngine &lua_engine, String path);

}
