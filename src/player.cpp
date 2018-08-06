#include <algorithm>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/util/log.hpp>
#include <lux/common/entity.hpp>
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
        sp.init.tick_rate = conf.tick_rate;
        sp.init.tick_rate = conf.tick_rate;
        std::copy(conf.server_name.begin(), conf.server_name.end(),
                  std::back_inserter(sp.conf.server_name));
        sent_init = true;
        return true;
    }
    return false;
}

Entity &Player::get_entity()
{
    return *entity;
}
