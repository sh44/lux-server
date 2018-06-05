#pragma once

#include <atomic>
#include <thread>
//
#include <enet/enet.h>
//
#include <alias/string.hpp>
#include <alias/hash_map.hpp>
#include <alias/ref.hpp>
#include <data/config.hpp>
#include <util/tick_clock.hpp>
#include <net/ip.hpp>
#include <net/port.hpp>
#include <player.hpp>
#include <world.hpp>

class Server
{
public:
    Server(net::Port port, double tick_rate);
    Server(Server const &that) = delete;
    Server &operator=(Server const &that) = delete;
    ~Server();

    void start();
    void stop();

    void kick_player(net::Ip ip, String reason = "unknown");
private:
    enum State
    {
        NOT_STARTED,
        RUNNING,
        STOPPED
    };
    const SizeT MAX_CLIENTS = 16;

    void run();
    void tick();
    void handle_input();
    void handle_output();
    void disconnect_player(net::Ip ip);
    void add_player(ENetPeer *peer);

    std::thread        thread;
    std::atomic<State> state;
    ENetAddress        enet_address;
    ENetHost          *enet_server;
    util::TickClock    tick_clock;
    data::Config       config;
    world::World       world;

    HashMap<net::Ip, Player> players; //TODO reference to player ip?
};
