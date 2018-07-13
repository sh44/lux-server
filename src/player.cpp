#include <alias/int.hpp>
#include <tile/tile_type.hpp>
#include <map/common.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
#include <net/server_data.hpp>
#include <net/client_data.hpp>
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
    deserializer.pop(cd);
    view_size = cd.view_size;
}

ENetPacket *Player::send() const
{
    Vector<net::TileState> tiles;
    tiles.reserve(view_size.x * view_size.y);
    MapPoint offset;
    MapPoint entity_pos = (MapPoint)entity->get_pos();
    offset.z = 0;
    for(offset.y = -(int)((view_size.y + 1) / 2); offset.y < view_size.y / 2; ++offset.y)
    {
        for(offset.x = -(int)((view_size.x + 1) / 2); offset.x < view_size.x / 2; ++offset.x)
        {
            MapPoint tile_pos = entity_pos + offset;
            auto const &tile_type = entity->world[tile_pos].type;
            tiles.emplace_back();
            tiles.back().shape = (net::TileState::Shape)tile_type->shape;
            tiles.back().tex_pos = tile_type->tex_pos;
        }
    }
    sd.tiles = tiles; //TODO differential copy?
    net::Serializer serializer(tiles.size() * sizeof(net::TileState));
    serializer.push(sd);
    return enet_packet_create(serializer.get(), serializer.get_size(), 0);
}
