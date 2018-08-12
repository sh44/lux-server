#include <algorithm>
//
#include <glm/glm.hpp>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/util/log.hpp>
#include <lux/common/entity.hpp>
#include <lux/common/map.hpp>
#include <lux/net/server/packet.hpp>
#include <lux/net/client/packet.hpp>
//
#include <map/tile_type.hpp>
#include <entity.hpp>
#include <world.hpp>
#include "player.hpp"

Player::Player(data::Config const &conf, ENetPeer *peer, Entity &entity) :
    peer(peer),
    conf(conf),
    entity(&entity),
    view_range(0, 0, 0),
    sent_init(false),
    received_init(false)
{

}

void Player::receive(net::client::Packet const &cp)
{
    if(!received_init)
    {
        if(cp.type == net::client::Packet::INIT)
        {
            init_from_client(cp.init);
            received_init = true;
        }
        else
        {
            throw std::runtime_error("client has not sent init data");
            //TODO just kick him out
        }
    }
    else if(cp.type == net::client::Packet::TICK)
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
    if(!sent_init)
    {
        util::log("PLAYER", util::INFO, "initializing to client");
        sp.type = net::server::Packet::INIT;
        sp.init.conf.tick_rate = conf.tick_rate; //TODO Player::prepare_conf?
        std::copy(conf.server_name.begin(), conf.server_name.end(),
                  std::back_inserter(sp.init.server_name));
        sp.init.chunk_size = CHK_SIZE;
        sent_init = true;
        return true;
    }
    else
    {
        ChkPos iter;
        ChkPos center = to_chk_pos(glm::round(entity->get_pos()));
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

void Player::send_chunk(net::server::Packet &sp, ChkPos const &pos)
{
    sp.type = net::server::Packet::MAP;
    auto &chunk = sp.map.chunks.emplace_back();
    auto const &world_chunk = entity->world.get_chunk(pos);
    chunk.pos = pos;
    for(ChkIdx i = 0; i < CHK_VOLUME; ++i)
    {
        chunk.tiles[i].db_hash = std::hash<String>()(world_chunk.tiles[i]->id);
    }
}

void Player::init_from_client(net::client::Init const &ci)
{
    util::log("PLAYER", util::INFO, "received initialization data");
    String client_name(ci.client_name.begin(), ci.client_name.end());
    util::log("PLAYER", util::INFO, "client name: %s", client_name);
    change_config(ci.conf);
}

void Player::change_config(net::client::Conf const &cc)
{
    view_range = cc.view_range;
}

Entity &Player::get_entity()
{
    return *entity;
}
