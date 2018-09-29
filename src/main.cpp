#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstdlib>
#include <cmath>
//
#include <lua.hpp>
//
#include <lux_shared/common.hpp>
#include <lux_shared/util/tick_clock.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>
#include <server.hpp>

std::atomic<bool> exiting = false;

lua_State* lua_L;

Queue<String> command_queue;
std::mutex command_queue_mutex;

void parse_command(char const* command) {
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

void add_command(char const* str) {
    command_queue_mutex.lock();
    command_queue.emplace(str);
    command_queue_mutex.unlock();
}

void console_main() {
    std::string input;
    while(!exiting) {
        std::getline(std::cin, input);
        if(input.size() > 2 && input[0] == '!') {
            input.erase(0, 1);
            add_command(input.c_str());
        } else {
            server_broadcast(input.c_str());
        }
    }
}

int main(int argc, char** argv) {
    U16 server_port;
    { ///read commandline args
        if(argc != 2) {
            LUX_FATAL("usage: %s SERVER_PORT", argv[0]);
        }
        U64 raw_server_port = std::atol(argv[1]);
        if(raw_server_port >= 1 << 16) {
            LUX_FATAL("invalid port %zu given", raw_server_port);
        }
        server_port = raw_server_port;
    }

    db_init();
    constexpr F64 TICK_RATE = 64.0;
    server_init(server_port, TICK_RATE);
    LUX_DEFER { server_deinit(); };

    {
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
    LUX_DEFER { lua_close(lua_L); };

    std::thread console_thread = std::thread(&console_main);
    { ///main loop
        auto tick_len = util::TickClock::Duration(1.0 / TICK_RATE);
        util::TickClock clock(tick_len);
        while(server_is_running()) {
            clock.start();
            entities_tick();
            static DynArr<ChkPos> light_updated_chunks;
            map_tick(light_updated_chunks);

            server_tick(light_updated_chunks);
            light_updated_chunks.clear();

            command_queue_mutex.lock();
            while(!command_queue.empty()) {
                parse_command(command_queue.front().c_str());
                command_queue.pop();
            }
            command_queue_mutex.unlock();
            clock.stop();
            auto remaining = clock.synchronize();
            if(remaining < util::TickClock::Duration::zero()) {
                LUX_LOG("tick overhead of %.2fs", std::abs(remaining.count()));
            }
        }
    }
    exiting = true;
    console_thread.join();
    return 0;
}
