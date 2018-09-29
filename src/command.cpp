#include <mutex>
//
#include <lua.hpp>
//
#include <lux_shared/common.hpp>
//
#include "command.hpp"

lua_State* lua_L;

Queue<String> command_queue;
std::mutex command_queue_mutex;

void command_init() {
    lua_L = luaL_newstate();
    luaL_openlibs(lua_L);
    luaL_loadstring(lua_L, "package.path = package.path .. \";api/?.lua\"");
    lua_call(lua_L, 0, 0);
    switch(luaL_loadfile(lua_L, "api/lux-api.lua")) {
        case 0: break;
        case LUA_ERRSYNTAX:
            LUX_FATAL("lua syntax error: \n%s",
                      lua_tolstring(lua_L, -1, nullptr));
        case LUA_ERRMEM: LUX_FATAL("lua memory error");
        default: LUX_FATAL("lua unknown error");
    }
    switch(lua_pcall(lua_L, 0, 0, 0)) {
        case 0: break;
        case LUA_ERRRUN:
            LUX_FATAL("lua runtime error: \n%s",
                      lua_tolstring(lua_L, -1, nullptr));
        case LUA_ERRMEM: LUX_FATAL("lua memory error");
        case LUA_ERRERR: LUX_FATAL("lua error handler error");
        default: LUX_FATAL("lua unknown error");
    }
}

void command_deinit() {
    lua_close(lua_L);
}

static void parse_command(char const* command) {
    switch(luaL_loadstring(lua_L, command)) {
        case 0: break;
        case LUA_ERRSYNTAX: {
            LUX_LOG("%s", lua_tolstring(lua_L, -1, nullptr));
            return;
        } break;
        case LUA_ERRMEM: LUX_FATAL("lua memory error");
        default: {
            LUX_LOG("unknown lua error");
            return;
        } break;
    }
    switch(lua_pcall(lua_L, 0, 0, 0)) {
        case 0: break;
        case LUA_ERRRUN: {
            LUX_LOG("%s", lua_tolstring(lua_L, -1, nullptr));
        } break;
        case LUA_ERRMEM: LUX_FATAL("lua memory error");
        case LUA_ERRERR: LUX_FATAL("lua error handler error");
        default: {
            LUX_LOG("unknown lua error");
        } break;
    }
}

void parse_command_queue() {
    command_queue_mutex.lock();
    while(!command_queue.empty()) {
        parse_command(command_queue.front().c_str());
        command_queue.pop();
    }
    command_queue_mutex.unlock();
}

void add_command(char const* str) {
    command_queue_mutex.lock();
    command_queue.emplace(str);
    command_queue_mutex.unlock();
}

