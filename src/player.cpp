#include <cassert>
#include <chrono>
#include <thread>
#include <algorithm>
//
#include <glm/glm.hpp>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common.hpp>
#include <lux/net/common.hpp>
#include <lux/world/entity.hpp>
#include <lux/world/map.hpp>
//
#include <map/voxel_type.hpp>
#include <entity/entity.hpp>
#include <world.hpp>
#include <net_buffers.hpp>
#include "player.hpp"

Player::Player(data::Config const &conf, ENetPeer *peer,
               Entity &entity, NetBuffers &nb) :
    peer(peer),
    conf(conf),
    entity(&entity),
    load_range(1.f),
    nb(nb)
{
    LUX_LOG("PLAYER", DEBUG, "initializing from client");
    if(receive_init()) {
        handle_init();
    } else {
        LUX_LOG("PLAYER", FATAL, "client has not sent init data");
        //TODO just kick him out
    }

    LUX_LOG("PLAYER", DEBUG, "initializing to client");
    prepare_init();
    send_init();
}

void Player::net_input_tick()
{
    receive_packets();
    while(!in_signal_buffers.empty()) {
        deserialize_packet(in_signal_buffers.front(), nb.cs);
        in_signal_buffers.pop();
        handle_signal();
    }
    if(in_tick_buffer != nullptr) {
        deserialize_packet(in_tick_buffer, nb.ct);
        handle_tick();
    } else {
        LUX_LOG("PLAYER", WARN, "tick packet lost");
    }
}

void Player::net_output_tick()
{
    while(prepare_signal()) {
        send_signal();
    }
    prepare_tick();
    send_tick();
}

void Player::receive_packets()
{
    in_tick_buffer = nullptr;
    assert(in_signal_buffers.empty());

    U8 channel_id;
    ENetPacket *pack = enet_peer_receive(peer, &channel_id);
    while(pack != nullptr) {
        switch(channel_id) {
        case net::TICK_CHANNEL:   in_tick_buffer = pack; break;
        case net::SIGNAL_CHANNEL: in_signal_buffers.push(pack); break;
        default: LUX_LOG("PLAYER", WARN,
                         "received packet on unknown channel %d", channel_id);
        }
        pack = enet_peer_receive(peer, &channel_id);
    }
}


bool Player::receive_init()
{
    constexpr std::chrono::duration<F64> SLEEP_DURATION(0.050);
    constexpr UInt MAX_TRIES = 10;

    for(UInt tries = 0; tries < MAX_TRIES; ++tries) {
        U8 channel_id;
        ENetPacket *pack = enet_peer_receive(peer, &channel_id);
        if(pack != nullptr && channel_id == net::INIT_CHANNEL) {
            serialize_packet(pack, nb.ci);
            return true;
        }
        std::this_thread::sleep_for(SLEEP_DURATION);
    }
    return false;
}

void Player::send_packet(U8 channel_id, ENetPacket *pack)
{
    if(enet_peer_send(peer, channel_id, pack) < 0) {
        LUX_LOG("PLAYER", WARN, "failed to send tick packet");
    }
}

void Player::send_tick()
{
    serialize_packet(nb.unreliable_packet, nb.st);
    send_packet(net::TICK_CHANNEL, nb.unreliable_packet);
}

void Player::send_signal()
{
    serialize_packet(nb.unreliable_packet, nb.ss);
    send_packet(net::SIGNAL_CHANNEL, nb.reliable_packet);
}

void Player::send_init()
{
    serialize_packet(nb.unreliable_packet, nb.si);
    send_packet(net::INIT_CHANNEL, nb.reliable_packet);
}

void Player::handle_tick()
{
    auto h_dir = 0.2f * nb.ct.character_dir;
    if(nb.ct.is_moving)  entity->move({h_dir.x, h_dir.y, 0.0});
    if(nb.ct.is_jumping) entity->jump();
}

void Player::handle_signal()
{
    switch(nb.cs.type) {
        case net::client::Signal::CONF: handle_conf(); break;
    }
}

void Player::handle_init()
{
    net::client::Init const &ci = nb.ci;
    LUX_LOG("PLAYER", INFO, "received initialization data");
    String client_name(ci.client_name.begin(), ci.client_name.end());
    LUX_LOG("PLAYER", INFO, "client name: %s", client_name);
}

void Player::handle_conf()
{
    net::client::Conf const &cc = nb.cs.conf;
    LUX_LOG("PLAYER", INFO, "changing config");
    LUX_LOG("PLAYER", INFO, "load range: %.2f", cc.load_range);
    load_range = cc.load_range;
}

void Player::prepare_tick()
{
    entity->world.get_entities_positions(nb.st.entities);
    nb.st.player_pos = entity->get_pos();
}

bool Player::prepare_signal()
{
    if(prepare_map_signal()) return true;
    return false;
}

bool Player::prepare_map_signal()
{
    bool is_sending = false;
    ChkPos iter;
    ChkPos center = to_chk_pos(glm::round(entity->get_pos()));
    for(iter.z = center.z - load_range;
        iter.z <= center.z + load_range;
        ++iter.z)
    {
        for(iter.y = center.y - load_range;
            iter.y <= center.y + load_range;
            ++iter.y)
        {
            for(iter.x = center.x - load_range;
                iter.x <= center.x + load_range;
                ++iter.x)
            {
                if(loaded_chunks.count(iter) == 0)
                {
                    if(glm::distance((Vec3F)iter, (Vec3F)center)
                           <= load_range)
                    {
                        entity->world.guarantee_chunk(iter);
                        auto const &chunk =
                            entity->world.get_chunk(iter);
                        prepare_chunk(chunk, iter);
                        loaded_chunks.insert(iter);
                        is_sending = true;
                    }
                }
            }
        }
    }
    if(is_sending) nb.ss.type = net::server::Signal::MAP;
    return is_sending;
}

void Player::prepare_chunk(Chunk const &world_chunk, ChkPos const &pos)
{
    auto &chunk = nb.ss.map.chunks.emplace_back();
    chunk.pos = pos;
    //TODO prevent copying?
    std::copy(world_chunk.voxels.cbegin(), world_chunk.voxels.cend(),
              chunk.voxels.begin());
    std::copy(world_chunk.light_lvls.cbegin(), world_chunk.light_lvls.cend(),
              chunk.light_lvls.begin());
}

void Player::prepare_init()
{
    net::server::Init &si = nb.si;
    si.tick_rate = conf.tick_rate;
    std::copy(conf.server_name.cbegin(), conf.server_name.cend(),
              std::back_inserter(si.server_name));
    si.chunk_size = CHK_SIZE;
}

Entity &Player::get_entity()
{
    return *entity;
}
