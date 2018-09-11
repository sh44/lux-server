#pragma once

#include <atomic>
#include <thread>
//
#include <enet/enet.h>
//
#include <lux/alias/string.hpp>
#include <lux/alias/hash_map.hpp>
#include <lux/common.hpp>
#include <lux/util/tick_clock.hpp>
#include <lux/net/common.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <net_buffers.hpp>
#include <player.hpp>
#include <world.hpp>

class Server
{
public:
    Server(net::Port port);
    LUX_NO_COPY(Server);
    ~Server();

    void kick_player(net::Ip ip, String const &reason = "unknown");
private:
    const SizeT MAX_CLIENTS = 16;

    void run();
    void tick();

    void net_input_tick();
    void net_output_tick();

    void erase_player(net::Ip ip);
    void add_player(ENetPeer *peer);
    void kick_all(String const &reason);

    std::thread        thread;
    ENetAddress        enet_address; //TODO is this needed?
    ENetHost          *enet_server;
    data::Database     db;
    data::Config       config;
    util::TickClock    tick_clock;
    World              world;
    std::atomic<bool>  should_close;

    HashMap<net::Ip, Player> players; //TODO reference to player ip?

    NetBuffers nb;
};
