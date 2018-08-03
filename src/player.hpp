#pragma once

#include <enet/enet.h>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/linear/vec_2.hpp>
#include <lux/serial/serializer.hpp>
#include <lux/serial/deserializer.hpp>
#include <lux/serial/server_init_data.hpp>
#include <lux/serial/server_data.hpp>
#include <lux/serial/client_data.hpp>

namespace data { struct Config; }

class Entity;

class Player
{
    public:
    Player(data::Config const &conf, ENetPeer *peer, Entity &entity);
    Player(Player const &that) = delete;
    Player &operator=(Player const &that) = delete;

    ENetPeer *peer;

    void receive(ENetPacket *packet);
    void send() const;
    Entity &get_entity();
    private:
    void init_to_client(F64 tick_rate, String const &server_name);
    linear::Vec2<U16> view_size; //in tiles
    Entity *entity;

    serial::ClientData   cd;
    serial::Deserializer deserializer;

    serial::ServerData mutable sd;
    serial::Serializer mutable serializer;
    // ^ this is mutable, because it would be normally allocated in send()
    //   everytime, instead it is hold here as a buffer to optimize things
};
