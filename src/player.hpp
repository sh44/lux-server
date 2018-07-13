#pragma once

#include <enet/enet.h>
//
#include <alias/int.hpp>
#include <linear/size_2d.hpp>

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
    linear::Size2d<U16> view_size; //in tiles
    Entity *entity;
};
