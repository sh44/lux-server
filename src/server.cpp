#include <stdexcept>
//
#include <lux/util/log.hpp>
//
#include <player.hpp>
#include <data/obj.hpp>
#include "server.hpp"

Server::Server(net::Port port) :
    enet_address({ENET_HOST_ANY, port}),
    enet_server(enet_host_create(&enet_address, MAX_CLIENTS, 2, 0, 0)),
    config({&db, &db.get_entity("human"), "lux-server", 64.0}),
    tick_clock(util::TickClock::Duration(1.0 / config.tick_rate)),
    world(config),
    should_close(false)
{
    if(enet_server == nullptr)
    {
        throw std::runtime_error("couldn't create ENet server host");
    }
    util::log("SERVER", util::INFO, "starting server");
    thread = std::thread(&Server::run, this);
}

Server::~Server()
{
    util::log("SERVER", util::INFO, "stopping server");
    should_close = true;
    thread.join();
    enet_host_destroy(enet_server);
}

void Server::kick_player(net::Ip ip, String const &reason)
{
    util::log("SERVER", util::INFO, "kicking player %u.%u.%u.%u, reason: %s",
              ip & 0xFF,
             (ip >>  8) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >> 24) & 0xFF,
             reason);
    ENetPeer *peer = players.at(ip).peer;
    enet_peer_disconnect(peer, 0);
    enet_host_flush(enet_server);
    //TODO kick message?
    enet_peer_reset(peer);
    erase_player(ip);
}

void Server::send_msg(net::Ip ip, String const &msg)
{
    util::log("SERVER", util::INFO, "sending msg to player %u.%u.%u.%u",
              ip & 0xFF,
             (ip >>  8) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >> 24) & 0xFF);
    util::log("SERVER", util::INFO, "msg: %s", msg);
    sp.type = net::server::Packet::MSG;
    sp.msg.log_level = util::INFO;
    std::copy(msg.begin(), msg.end(), std::back_inserter(sp.msg.log_msg));
    send_server_packet(players.at(ip), ENET_PACKET_FLAG_RELIABLE);
    enet_host_flush(enet_server);
}

void Server::kick_all(String const &reason)
{
    util::log("SERVER", util::INFO, "kicking all, reason: %s", reason);
    auto iter = players.begin();
    while(iter != players.end())
    {
        kick_player(iter->first, reason);
        iter = players.begin();
    }
}

void Server::run()
{
    while(!should_close)
    {
        tick_clock.start();
        tick();
        tick_clock.stop();
        auto delta = tick_clock.synchronize();
        if(delta < util::TickClock::Duration::zero())
        {
            util::log("SERVER", util::WARN, "tick overhead of %.2f ticks",
                      std::abs(delta / tick_clock.get_tick_len()));
        }
    }
    kick_all("server stopping");
}

void Server::tick()
{
    world.update();
    handle_input();
    handle_output();
}

void Server::handle_input()
{
    ENetEvent event;
    while(enet_host_service(enet_server, &event,
                            tick_clock.get_tick_len().count() / 1000.f) > 0)
    {
        if(event.type == ENET_EVENT_TYPE_CONNECT)
        {
            add_player(event.peer);
        }
        else if(event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            auto *pack = event.packet;
            deserializer.set_slice(pack->data, pack->data + pack->dataLength);
            net::clear_buffer(cp);
            deserializer >> cp;
            players.at(event.peer->address.host).receive(cp);
            enet_packet_destroy(pack);
        }
        else if(event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            erase_player(event.peer->address.host);
        }
    }
}

void Server::handle_output()
{
    for(auto &player : players)
    {
        while(player.second.send_signal(sp))
        {
            send_server_packet(player.second, ENET_PACKET_FLAG_RELIABLE);
        }
        player.second.send_tick(sp);
        send_server_packet(player.second, ENET_PACKET_FLAG_UNSEQUENCED);
    }
}

void Server::erase_player(net::Ip ip)
{
    players.at(ip).get_entity().deletion_mark = true;
    players.erase(ip);
    util::log("SERVER", util::INFO, "player %u.%u.%u.%u disconnected",
              ip & 0xFF,
             (ip >>  8) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >> 24) & 0xFF);
}

void Server::add_player(ENetPeer *peer)
{
    net::Ip ip = peer->address.host;
    if(players.count(ip) > 0)
    {
        kick_player(ip, "double join");
    }
    util::log("SERVER", util::INFO, "player %u.%u.%u.%u connected",
              ip & 0xFF,
             (ip >>  8) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >> 24) & 0xFF);
    players.emplace(std::piecewise_construct,
                    std::forward_as_tuple(peer->address.host),
                    std::forward_as_tuple(config, peer, world.create_player()));
    /* why you so ugly C++? */
}

void Server::send_server_packet(Player const &player, U32 flags)
{
    //TODO packet buffer with enet_packet_resize
    serializer.reserve(net::get_size(sp));
    serializer << sp;
    ENetPacket *pack =
        enet_packet_create(serializer.get(), serializer.get_used(), flags);
    enet_peer_send(player.peer, 0, pack);
    enet_host_flush(enet_server);
    net::clear_buffer(sp);
}

