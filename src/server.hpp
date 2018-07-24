#pragma once

#include <atomic>
#include <thread>
//
#include <enet/enet.h>
//
#include <lux/alias/string.hpp>
#include <lux/alias/hash_map.hpp>
#include <lux/alias/ref.hpp>
#include <lux/util/tick_clock.hpp>
#include <lux/net/ip.hpp>
#include <lux/net/port.hpp>
//
#include <data/config.hpp>
#include <player.hpp>
#include <world.hpp>

class Server
{
public:
    Server(net::Port port);
    Server(Server const &that) = delete;
    Server &operator=(Server const &that) = delete;
    ~Server();

    void kick_player(net::Ip ip, String const &reason = "unknown");
private:
    const SizeT MAX_CLIENTS = 16;

    void run();
    void tick();
    void handle_input();
    void handle_output();
    void disconnect_player(net::Ip ip);
    void add_player(ENetPeer *peer);

    std::thread        thread;
    ENetAddress        enet_address;
    ENetHost          *enet_server;
    data::Config       config;
    util::TickClock    tick_clock;
    World              world;
    std::atomic<bool>  should_close;

    HashMap<net::Ip, Player> players; //TODO reference to player ip?
};
