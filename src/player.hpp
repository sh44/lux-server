#pragma once

#include <enet/enet.h>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/alias/set.hpp>
#include <lux/alias/queue.hpp>
#include <lux/common.hpp>
#include <lux/world/map.hpp>
//
#include <net_buffers.hpp>

namespace data { struct Config; }

struct Chunk;
class Entity;

class Player
{
    public:
    Player(data::Config const &conf, ENetPeer *peer,
           Entity &entity, NetBuffers &nb);
    LUX_NO_COPY(Player);

    void net_input_tick();
    void net_output_tick();

    Entity &get_entity();

    ENetPeer *peer;
    private:
    template<typename Buff>
    void deserialize_packet(ENetPacket *pack, Buff &buff);
    template<typename Buff>
    void serialize_packet(ENetPacket *pack, Buff &buff);

    void receive_packets();
    bool receive_tick();
    bool receive_signal();
    bool receive_init();

    void send_packet(U8 channel_id, ENetPacket *pack);
    void send_tick();
    void send_signal();
    void send_init();

    void handle_tick();
    void handle_signal();
    void handle_conf();
    void handle_init();

    void prepare_tick();
    bool prepare_signal();
    bool prepare_map_signal();
    void prepare_chunk(Chunk const &, ChkPos const &pos);
    void prepare_init();

    data::Config const &conf;

    //TODO style change, const &T to const& T
    ENetPacket *        in_tick_buffer;
    Queue<ENetPacket *> in_signal_buffers;

    Set<ChkPos> loaded_chunks;
    Entity *entity;
    F32 load_range;

    NetBuffers &nb;
};

template<typename Buff>
void Player::deserialize_packet(ENetPacket *pack, Buff &buff)
{
    nb.deserializer.set_slice(pack->data,
                              pack->data + pack->dataLength);
    net::clear_buffer(buff);
    nb.deserializer >> buff;
    enet_packet_destroy(pack);
}

template<typename Buff>
void Player::serialize_packet(ENetPacket *pack, Buff &buff)
{
    nb.serializer.reserve(net::get_size(buff));
    nb.serializer << buff;
    net::clear_buffer(buff);
    //TODO for some reason data is not transferred correctly when
    //ENET_PACKET_FLAG_NO_ALLOCATE is set
    //without it it's quite slow, the data is allocated per-packet instead of
    //using a buffer, and also it is copied
    if(enet_packet_resize(pack, nb.serializer.get_used()) < 0) {
        LUX_LOG("PLAYER", FATAL, "packet resize failed");
    }
    pack->data = (enet_uint8 *)nb.serializer.get();
}
