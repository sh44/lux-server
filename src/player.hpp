#pragma once

#include <enet/enet.h>
//
#include <lux/alias/set.hpp>
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/map.hpp>

namespace data { struct Config; }
namespace net::client
{
    struct Packet;
    struct Init;
    struct Conf;
}
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
    void send_chunk(net::server::Packet &, ChkPos const &pos);
    Entity &get_entity();

    ENetPeer *peer;
    private:
    void init_from_client(net::client::Init const &);
    void change_config(net::client::Conf const &);

    data::Config const &conf;

    Set<ChkPos> loaded_chunks;
    Entity *entity;
    F32 load_range;

    bool sent_init;
    bool received_init;
};
