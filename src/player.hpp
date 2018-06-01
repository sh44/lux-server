#pragma once

#include <queue>
//
#include <enet/enet.h>
//
#include <linear/size_2d.hpp>

namespace world { inline namespace entity { class Entity; } }

class Player
{
    public:
    Player(ENetPeer *peer, world::Entity &entity);

    ENetPeer *peer;

    void receive(ENetPacket *packet);
    ENetPacket *send();
    private:
    linear::Size2d<uint16_t> window_size; //in tiles
    world::Entity *entity;
};
