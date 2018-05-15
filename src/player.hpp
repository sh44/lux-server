#pragma once

#include <queue>
//
#include <enet/enet.h>

namespace world { class Entity; }

class Player
{
    public:
    Player(ENetPeer *peer, world::Entity &entity);

    ENetPeer *peer;

    void receive(ENetPacket *packet);
    void update();
    private:
    world::Entity *entity;
    std::queue<ENetPacket *> packet_queue;
};
