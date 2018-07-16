#include <alias/scalar.hpp>
#include <tile/tile_type.hpp>
#include <common/entity.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
#include <net/server/server_data.hpp>
#include <net/client/client_data.hpp>
#include "player.hpp"

Player::Player(ENetPeer *peer, Entity &entity) :
    peer(peer),
    entity(&entity)
{

}

void Player::receive(ENetPacket *packet)
{
    assert(packet->dataLength >= sizeof(net::ClientData));
    net::Deserializer deserializer(packet->data, packet->data + packet->dataLength);
    deserializer >> cd;
    auto h_dir = 0.2f * cd.character_dir;
    if(cd.is_moving) entity->move({h_dir.x, h_dir.y, 0.0});
}

ENetPacket *Player::send() const
{
    //sd.chunk_data
    //entity->world.get_entities_positions(sd.entities);
    sd.player_pos = entity->get_pos();
    net::Serializer serializer(0);
    return enet_packet_create(serializer.get(), serializer.get_size(), 0);
}
