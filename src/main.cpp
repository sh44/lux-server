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
#include <command.hpp>

std::atomic<bool> exiting = false;

void console_main() {
    std::string input;
    while(!exiting) {
        std::getline(std::cin, input);
        if(input.size() > 2 && input[0] == '!') {
            input.erase(0, 1);
            add_command(input.c_str());
        } else if(input.size() > 1) {
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
    command_init();
    LUX_DEFER { command_deinit(); };
    constexpr F64 TICK_RATE = 64.0;
    server_init(server_port, TICK_RATE);
    LUX_DEFER { server_deinit(); };

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

            parse_command_queue();
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
