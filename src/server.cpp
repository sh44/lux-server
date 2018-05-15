#include <ncurses.h>
#include <cstdint>
#include <stdexcept>
//
#include <player.hpp>
#include "server.hpp"

Server::Server(uint16_t port, double tick_rate) :
    state(NOT_STARTED),
    enet_address({ENET_HOST_ANY, port}),
    enet_server(enet_host_create(&enet_address, MAX_CLIENTS, 1, 0, 0)),
    tick_clock(util::TickClock::Duration(1.0 / tick_rate))
{
    if(enet_server == NULL)
    {
        throw std::runtime_error("couldn't create ENet server host");
    }
}

Server::~Server()
{
    enet_host_destroy(enet_server);
}

void Server::start()
{
    switch(state)
    {
        case NOT_STARTED: state = RUNNING; break;
        case RUNNING: throw std::runtime_error("server has already been started");
        case STOPPED: state = RUNNING; break;
    }
    thread = std::thread(&Server::run, this);
}

void Server::stop()
{
    switch(state)
    {
        case RUNNING: state = STOPPED; break;
        case NOT_STARTED: throw std::runtime_error("tried to stop a server that hasn't been started yet");
        case STOPPED: throw std::runtime_error("tried to stop a stopped server");
    }
    thread.join();
}

void Server::kick_player(net::Ip ip, std::string reason)
{
    ENetPeer *peer = players.at(ip).peer;
    enet_peer_disconnect(peer, (uint32_t)(std::size_t)("KICKED FOR: " + reason).c_str());
    /* uint32_t is smaller than a pointer, so it's probably a bad idea,
     * but that's how ENet does it, unless I misunderstood something,
     * and the uint32_t value shouldn't really be a pointer
     */
    enet_peer_reset(peer);
}

void Server::run()
{
    while(state == RUNNING)
    {
        tick_clock.start();
        tick();
        tick_clock.stop();
        tick_clock.synchronize();
    }
}

void Server::tick()
{
    handle_events();
}

void Server::handle_events()
{
    ENetEvent event;
    while(enet_host_service(enet_server, &event, 0) > 0);
    switch(event.type)
    {
        case ENET_EVENT_TYPE_NONE: break;

        case ENET_EVENT_TYPE_CONNECT: add_player(event.peer); break;

        case ENET_EVENT_TYPE_RECEIVE: players.at(event.peer->address.host)
                                             .receive(event.packet); break;

        case ENET_EVENT_TYPE_DISCONNECT: disconnect_player(event.peer->address.host); break;
    }
}

void Server::disconnect_player(net::Ip ip)
{
    players.erase(ip);
}

void Server::add_player(ENetPeer *peer)
{
    players.insert({peer->address.host, {peer, world.create_player()}});
}
