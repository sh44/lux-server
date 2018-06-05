#pragma once

#include <enet/enet.h>
//
#include <alias/int.hpp>
#include <linear/size_2d.hpp>

namespace world { class Entity; }

class Player
{
    public:
    Player(ENetPeer *peer, world::Entity &entity);

    ENetPeer *peer;

    void receive(ENetPacket *packet);
    ENetPacket *send() const;
    private:
    linear::Size2d<U16> view_size; //in tiles
    world::Entity *entity;
};
