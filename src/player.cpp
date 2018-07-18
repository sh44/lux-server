#include <memory>
//
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
    net::Deserializer deserializer(packet->data, packet->data + packet->dataLength);
    deserializer >> cd;
    auto h_dir = 0.2f * cd.character_dir;
    if(cd.is_moving) entity->move({h_dir.x, h_dir.y, 0.0});
}

ENetPacket *Player::send() const
{
    sd.chunks.len = cd.chunk_requests.len;
    std::unique_ptr<net::ChunkData> chunks_ptr(new net::ChunkData[sd.chunks.len]);
    sd.chunks.val = chunks_ptr.get();
    for(SizeT i = 0; i < sd.chunks.len; ++i)
    {
        sd.chunks.val[i].pos = cd.chunk_requests.val[i];
        for(SizeT j = 0; j < consts::CHUNK_TILE_SIZE; ++j)
        {
            MapPos map_pos = chunk_to_map_pos(sd.chunks.val[i].pos, j);
            sd.chunks.val[i].tiles[j].db_hash =
                std::hash<String>()(entity->world[map_pos].type->id);
        }
    }
    Vector<EntityPos> entities; //TODO
    entity->world.get_entities_positions(entities);
    std::unique_ptr<EntityPos> entities_ptr(new EntityPos[entities.size()]);
    sd.entities.val = entities_ptr.get();
    for(SizeT i = 0; i < entities.size(); ++i)
    {
        sd.entities.val[i] = entities[i];
    }
    sd.player_pos = entity->get_pos();
    net::Serializer serializer(sizeof(SizeT) * 2 + sizeof(net::ChunkData) * sd.chunks.len +
        sizeof(EntityPos) * sd.entities.len + sizeof(EntityPos));
    serializer << sd;
    return enet_packet_create(serializer.get(), serializer.get_size(), 0);
}
