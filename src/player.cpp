#include "player.hpp"

Player::Player(ENetPeer *peer, world::Entity &entity) :
    peer(peer),
    entity(&entity)
{

}

void Player::receive(ENetPacket *packet)
{
    packet_queue.push(packet);
}

void Player::update()
{
    if(!packet_queue.empty())
    {
        ENetPacket *packet = packet_queue.front();

        packet_queue.pop();
    }
}
