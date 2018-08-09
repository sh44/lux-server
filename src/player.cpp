#include <algorithm>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/util/log.hpp>
#include <lux/common/entity.hpp>
#include <lux/common/chunk.hpp>
#include <lux/net/server/packet.hpp>
#include <lux/net/client/packet.hpp>
//
#include <tile/type.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
#include "player.hpp"

Player::Player(data::Config const &conf, ENetPeer *peer, Entity &entity) :
    peer(peer),
    conf(conf),
    entity(&entity),
    sent_init(false)
{

}

void Player::receive(net::client::Packet const &cp)
{
    if(cp.type == net::client::Packet::TICK)
    {
        auto h_dir = 0.2f * cp.tick.character_dir;
        if(cp.tick.is_moving)  entity->move({h_dir.x, h_dir.y, 0.0});
        if(cp.tick.is_jumping) entity->jump();
    }
}

void Player::send_tick(net::server::Packet &sp) const
{
    sp.type = net::server::Packet::TICK;
    entity->world.get_entities_positions(sp.tick.entities); //TODO
    sp.tick.player_pos = entity->get_pos();
}

bool Player::send_signal(net::server::Packet &sp)
{
    if(sent_init == false)
    {
        util::log("PLAYER", util::INFO, "initializing to client");
        sp.type = net::server::Packet::INIT;
        sp.init.tick_rate = conf.tick_rate;
        std::copy(conf.server_name.begin(), conf.server_name.end(),
                  std::back_inserter(sp.init.server_name));
        sp.init.chunk_size = chunk::SIZE;
        sent_init = true;
        return true;
    }
    else
    {
        chunk::Pos iter;
        chunk::Pos center = chunk::to_pos(entity->get_pos()); //TODO entity::to_pos
        for(iter.z = center.z - view_range.z;
            iter.z <= center.z + view_range.z;
            ++iter.z)
        {
            for(iter.y = center.y - view_range.y;
                iter.y <= center.y + view_range.y;
                ++iter.y)
            {
                for(iter.x = center.x - view_range.x;
                    iter.x <= center.x + view_range.x;
                    ++iter.x)
                {
                    if(loaded_chunks.count(iter) == 0)
                    {
                        send_chunk(sp, iter);
                        loaded_chunks.insert(iter);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Player::send_chunk(net::server::Packet &sp, chunk::Pos const &pos)
{
    sp.type = net::server::Packet::MAP;
    auto &chunk = sp.map.chunks.emplace_back();
    auto const &world_chunk = entity->world[pos];
    chunk.pos = pos;
    for(chunk::Index i = 0; i < chunk::TILE_SIZE; ++i)
    {
        chunk.tiles[i].db_hash = std::hash<String>()(world_chunk.tiles[i].type->id);
    }
}

Entity &Player::get_entity()
{
    return *entity;
}
