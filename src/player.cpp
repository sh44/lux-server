#include <algorithm>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/util/log.hpp>
#include <lux/common/entity.hpp>
#include <lux/serial/server_data.hpp>
#include <lux/serial/client_data.hpp>
//
#include <tile/type.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
#include "player.hpp"

Player::Player(data::Config const &conf, ENetPeer *peer, Entity &entity) :
    peer(peer),
    entity(&entity)
{
    init_to_client(conf.tick_rate, conf.server_name);
}

void Player::receive(ENetPacket *packet)
{
    cd.chunk_requests.clear();
    deserializer.set_slice(packet->data, packet->data + packet->dataLength);
    deserializer >> cd;
    auto h_dir = 0.2f * cd.character_dir;
    if(cd.is_moving)  entity->move({h_dir.x, h_dir.y, 0.0});
    if(cd.is_jumping) entity->jump();
}

void Player::send() const
{
    sd.entities.clear(); //TODO not perfect
    sd.chunks.resize(cd.chunk_requests.size());
    for(SizeT i = 0; i < cd.chunk_requests.size(); ++i)
    {
        sd.chunks[i].pos = cd.chunk_requests[i];
        auto const &chunk = entity->world[sd.chunks[i].pos];
        for(SizeT j = 0; j < chunk::TILE_SIZE; ++j)
        {
            sd.chunks[i].tiles[j].db_hash =
                std::hash<String>()(chunk.tiles[j].type->id);
        }
    }
    entity->world.get_entities_positions(sd.entities);
    sd.player_pos = entity->get_pos();
    serializer.reserve(serial::get_size(sd));
    serializer << sd;
    ENetPacket *packet = enet_packet_create(serializer.get(),
        serializer.get_used(), 0);
    enet_peer_send(peer, 0, packet);
}

Entity &Player::get_entity()
{
    return *entity;
}

void Player::init_to_client(F64 tick_rate, String const &server_name)
{
    serial::ServerInitData sid;
    sid.tick_rate = tick_rate;
    std::copy(server_name.begin(), server_name.end(),
              std::back_inserter(sid.server_name));
    serializer.reserve(serial::get_size(sid));
    serializer << sid;
    ENetPacket *packet = enet_packet_create(serializer.get(),
        serializer.get_used(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

