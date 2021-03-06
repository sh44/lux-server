#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
//
#include <lux_shared/common.hpp>
#include <lux_shared/util/tick_clock.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>
#include <server.hpp>

std::atomic<bool> exiting(false);

void console_main() {
    /*std::string input;
    while(!exiting) {
        std::getline(std::cin, input);
        if(input.size() > 2 && input[0] == '/') {
            input.erase(0, 1);
            //@TODO add_command(input.c_str());
            LUX_UNIMPLEMENTED();
        } else if(input.size() > 1) {
            server_broadcast(input.c_str());
        }
    }*/
    LUX_UNIMPLEMENTED();
}

int main(int argc, char** argv) {
    random_seed = std::time(nullptr);

    U16 server_port = 31337;
    { ///read commandline args
        if(argc == 1) {
            LUX_LOG("no commandline arguments given");
            LUX_LOG("assuming server port %u", server_port);
        } else {
            if(argc != 2) {
                LUX_FATAL("usage: %s SERVER_PORT", argv[0]);
            }
            U64 raw_server_port = std::atol(argv[1]);
            if(raw_server_port >= 1 << 16) {
                LUX_FATAL("invalid port %zu given", raw_server_port);
            }
            server_port = raw_server_port;
        }
    }

    db_init();
    constexpr F64 TICK_RATE = 64.0;
    map_init();
    LUX_DEFER { map_deinit(); };
    physics_init();
    server_init(server_port, TICK_RATE);
    LUX_DEFER { server_deinit(); };
    LUX_LOG("chunk: %zu", sizeof(Chunk));
    LUX_LOG("chunk.data: %zu", sizeof(Chunk::Data));
    LUX_LOG("chunk.mesh: %zu", sizeof(ChunkMesh));

    std::thread console_thread = std::thread(&console_main);
    { ///main loop
        auto tick_len = util::TickClock::Duration(1.0 / TICK_RATE);
        util::TickClock clock(tick_len);
        while(server_is_running()) {
            clock.start();
            benchmark("entities tick", 1.0 / TICK_RATE, [&](){entities_tick();});
            benchmark("map tick"     , 1.0 / TICK_RATE, [&](){map_tick();});
            benchmark("server tick"  , 1.0 / TICK_RATE, [&](){server_tick();});

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
