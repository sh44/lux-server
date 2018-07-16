#pragma once

#include <enet/enet.h>
//
#include <alias/scalar.hpp>
#include <linear/vec_2.hpp>
#include <net/server/server_data.hpp>
#include <net/client/client_data.hpp>

class Entity;

class Player
{
    public:
    Player(ENetPeer *peer, Entity &entity);
    Player(Player const &that) = delete;
    Player &operator=(Player const &that) = delete;

    ENetPeer *peer;

    void receive(ENetPacket *packet);
    ENetPacket *send() const;
    private:
    linear::Vec2<U16> view_size; //in tiles
    Entity *entity;

    net::ClientData cd;
    net::ServerData mutable sd;
    // ^ this is mutable, because it would be normally allocated in send()
    //   everytime, instead it is hold here as a buffer to optimize things
};
