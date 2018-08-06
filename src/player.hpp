#pragma once

#include <enet/enet.h>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>

namespace data { struct Config; }
namespace net::client { struct Packet; }
namespace net::server { struct Packet; }

class Entity;

class Player
{
    public:
    Player(data::Config const &conf, ENetPeer *peer, Entity &entity);
    Player(Player const &that) = delete;
    Player &operator=(Player const &that) = delete;

    void receive(net::client::Packet const &);
    void send_tick(net::server::Packet &) const;
    bool send_signal(net::server::Packet &);
    Entity &get_entity();

    ENetPeer *peer;
    private:
    data::Config const &conf;
    Entity *entity;
    bool sent_init;
};
