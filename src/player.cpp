#include <lux/alias/scalar.hpp>
#include <lux/common/entity.hpp>
#include <lux/net/server/server_data.hpp>
#include <lux/net/client/client_data.hpp>
//
#include <tile/tile_type.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
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
    Vector<net::ChunkData> chunks;
    /*chunks.reserve(cd.requested_chunks.size());
    for(auto const &chunk : cd.requested_chunks)
    {
        //chunks.emplace_back(
    }
    //sd.chunks
    //entity->world.get_entities_positions(sd.entities);*/
    net::Serializer serializer(sizeof(net::ChunkData) * sd.chunks.len +
        sizeof(EntityPos) * sd.entities.len + sizeof(EntityPos));
    sd.player_pos = entity->get_pos();
    return enet_packet_create(serializer.get(), serializer.get_size(), 0);
}
