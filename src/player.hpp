#pragma once

#include <enet/enet.h>
//
#include <lux/alias/set.hpp>
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/chunk.hpp>

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
    void send_chunk(net::server::Packet &, chunk::Pos const &pos);
    Entity &get_entity();

    ENetPeer *peer;
    private:
    data::Config const &conf;

    Set<chunk::Pos> loaded_chunks;
    static constexpr Vec3<U16> view_range = {4, 4, 4}; //TODO get from client

    Entity *entity;
    bool sent_init;
};
