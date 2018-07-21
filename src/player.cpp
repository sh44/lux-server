#include <memory>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/entity.hpp>
#include <lux/serial/server_data.hpp>
#include <lux/serial/client_data.hpp>
//
#include <tile/type.hpp>
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
    deserializer.set_slice(packet->data, packet->data + packet->dataLength);
    deserializer >> cd;
    auto h_dir = 0.2f * cd.character_dir;
    if(cd.is_moving) entity->move({h_dir.x, h_dir.y, 0.0});
}

ENetPacket *Player::send() const
{
    serializer.clear();
    sd.chunks.resize(cd.chunk_requests.size());
    for(SizeT i = 0; i < cd.chunk_requests.size(); ++i)
    {
        sd.chunks[i].pos = cd.chunk_requests[i];
        for(SizeT j = 0; j < chunk::TILE_SIZE; ++j)
        {
            map::Pos map_pos = chunk::to_map_pos(sd.chunks[i].pos, j);
            sd.chunks[i].tiles[j].db_hash =
                std::hash<String>()(entity->world[map_pos].type->id);
        }
    }
    entity->world.get_entities_positions(sd.entities);
    sd.player_pos = entity->get_pos();
    serializer << sd;
    auto const &bytes = serializer.get();
    return enet_packet_create(bytes.data(), bytes.size(), 0);
}
