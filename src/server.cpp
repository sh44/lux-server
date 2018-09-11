#include <lux/util/log.hpp>
#include <lux/common.hpp>
//
#include <player.hpp>
#include "server.hpp"

Server::Server(net::Port port) :
    config({&db, db.get_entity_id("human"), "lux-server", 64.0}),
    tick_clock(util::TickClock::Duration(1.0 / config.tick_rate)),
    world(config),
    should_close(false)
{
    ENetAddress addr = {ENET_HOST_ANY, port};
    enet_server = enet_host_create(&addr, MAX_CLIENTS,
                                   net::CHANNEL_NUM, 0, 0);
    if(enet_server == nullptr) {
        LUX_LOG("SERVER", FATAL, "couldn't create ENet server host");
    }
    LUX_LOG("SERVER", INFO, "starting server");
    thread = std::thread(&Server::run, this);
}

Server::~Server()
{
    LUX_LOG("SERVER", INFO, "stopping server");
    should_close = true;
    thread.join();
    enet_host_destroy(enet_server);
}

void Server::kick_player(net::Ip ip, String const &reason)
{
    LUX_LOG("SERVER", INFO, "kicking player %u.%u.%u.%u, reason: %s",
            ((U8 *)&ip)[0], ((U8 *)&ip)[1], ((U8 *)&ip)[2], ((U8 *)&ip)[3],
            reason);
    ENetPeer *peer = players.at(ip).peer;
    enet_peer_disconnect(peer, 0);
    enet_host_flush(enet_server);
    //TODO kick message?
    enet_peer_reset(peer);
    erase_player(ip);
}

void Server::kick_all(String const &reason)
{
    LUX_LOG("SERVER", INFO, "kicking all, reason: %s", reason);
    auto iter = players.begin();
    while(iter != players.end()) {
        kick_player(iter->first, reason);
        iter = players.begin();
    }
}

void Server::run()
{
    while(!should_close) {
        tick_clock.start();
        tick();
        tick_clock.stop();
        auto delta = tick_clock.synchronize();
        if(delta < util::TickClock::Duration::zero()) {
            LUX_LOG("SERVER", WARN, "tick overhead of %.2f ticks",
                      std::abs(delta / tick_clock.get_tick_len()));
        }
    }
    kick_all("server stopping");
}

void Server::tick()
{
    world.tick();
    net_input_tick();
    net_output_tick();
}

void Server::net_input_tick()
{
    ENetEvent event;
    while(enet_host_service(enet_server, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_CONNECT) {
            add_player(event.peer);
        } else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
            erase_player(event.peer->address.host);
        }
        //we ignore ENET_EVENT_TYPE_RECEIVE, because the Player handles it
    }
    for(auto &player : players) {
        player.second.net_input_tick();
    }
}

void Server::net_output_tick()
{
    //TODO merge loops?
    for(auto &player : players) {
        player.second.net_output_tick();
    }
}

void Server::erase_player(net::Ip ip)
{
    players.at(ip).get_entity().deletion_mark = true;
    players.erase(ip);
    LUX_LOG("SERVER", INFO, "player %u.%u.%u.%u disconnected",
            ((U8 *)&ip)[0], ((U8 *)&ip)[1], ((U8 *)&ip)[2], ((U8 *)&ip)[3]);
}

void Server::add_player(ENetPeer *peer)
{
    net::Ip ip = peer->address.host;
    if(players.count(ip) > 0) {
        kick_player(ip, "double join");
    }
    LUX_LOG("SERVER", INFO, "player %u.%u.%u.%u connected",
            ((U8 *)&ip)[0], ((U8 *)&ip)[1], ((U8 *)&ip)[2], ((U8 *)&ip)[3]);
    /* why you so ugly C++? */
    players.emplace(std::piecewise_construct,
                    std::forward_as_tuple(peer->address.host),
                    std::forward_as_tuple(config, peer, world.create_player(),
                                          nb));
}
