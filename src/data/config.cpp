#include <stdexcept>
#include <lua.hpp>
//
#include <data/lua_engine.hpp>
#include "config.hpp"
#include <world/entity/type.hpp>

#include <util/log.hpp>
#include <data/database.hpp>

namespace data
{

Config load_config(LuaEngine &lua_engine, String path)
{
    switch(lua_engine.call(luaL_loadfile, path.c_str()))
    {
        case 0: break;
        case LUA_ERRSYNTAX:
            throw std::runtime_error(lua_engine.call(lua_tolstring, -1, (size_t *)NULL));
        case LUA_ERRMEM: throw std::bad_alloc();
    }
    switch(lua_engine.call(lua_pcall, 0, 0, 0)) //TODO internal lua memory leak here?
    {
        case 0: break;
        case LUA_ERRRUN:
            throw std::runtime_error(lua_engine.call(lua_tolstring, -1, (size_t *)NULL));
        case LUA_ERRMEM: throw std::bad_alloc();
        case LUA_ERRERR: throw std::runtime_error("error while running lua error handler");
    }
    Config result;
    lua_engine.call(lua_getfield, LUA_GLOBALSINDEX, "load_config");
    lua_engine.call(lua_pushlightuserdata, (void *)&result);
    switch(lua_engine.call(lua_pcall, 1, 0, 0))
    {
        case 0: break;
        case LUA_ERRRUN:
            throw std::runtime_error(lua_engine.call(lua_tolstring, -1, (size_t *)NULL));
        case LUA_ERRMEM: throw std::bad_alloc();
        case LUA_ERRERR: throw std::runtime_error("error while running load_config function");
    }
    util::log("CONFIG", util::TRACE, "%s\n", result.db->entity_type_at("human").id);
    return result;
}

}
