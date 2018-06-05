#include <alias/int.hpp>
#include <world/tile/type.hpp>
#include <world/map/common.hpp>
#include <world/entity.hpp>
#include <world.hpp>
#include <net/server_data.hpp>
#include <net/client_data.hpp>
#include "player.hpp"

Player::Player(ENetPeer *peer, world::Entity &entity) :
    peer(peer),
    entity(&entity)
{

}

void Player::receive(ENetPacket *packet)
{
    Vector<U8> bytes(packet->data, packet->data + packet->dataLength);
    net::ClientData client_data;
    net::ClientData::deserialize(client_data, bytes);
    view_size = client_data.view_size;
}

ENetPacket *Player::send() const
{
    //TODO store buffer in Player?
    Vector<net::TileState> tiles;
    tiles.reserve(view_size.x * view_size.y);
    for(SizeT i = 0; i < (SizeT)(view_size.x * view_size.y); ++i)
    {
        world::MapPoint offset = {i % view_size.x, i / view_size.x, 0};
        world::MapPoint tile_pos = (world::MapPoint)(entity->get_pos() + 0.5f) + offset;
        auto const &tile_type = entity->world[tile_pos].type;
        tiles.emplace_back((net::TileState::Shape)tile_type->shape,
                           tile_type->tex_pos);
    }
    net::ServerData server_data = {tiles};
    Vector<U8> bytes;
    net::ServerData::serialize(server_data, bytes);
    return enet_packet_create(bytes.data(), bytes.size(), 0);
}
