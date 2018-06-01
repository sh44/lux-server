#include <world/tile/type.hpp>
#include <world/map/point.hpp>
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
    std::vector<uint8_t> bytes(packet->data, packet->data + packet->dataLength);
    net::ClientData client_data = net::ClientData::deserialize(bytes);
    view_size = client_data.view_size;
}

ENetPacket *Player::send() const
{
    std::vector<world::tile::Type> tile_types;
    tile_types.reserve(view_size.x * view_size.y);
    for(std::size_t i = 0; i < view_size.x * view_size.y; ++i)
    {
        world::map::Point offset = {i % view_size.x, i / view_size.x, 0};
        world::map::Point tile_pos = (world::map::Point)(entity->get_pos() + 0.5f) + offset;
        tile_types.push_back(entity->world[tile_pos].type);
    }
    net::ServerData server_data = {tile_types};
    auto serialized = server_data.serialize();
    return enet_packet_create(serialized.data(), serialized.size(), 0);
}
