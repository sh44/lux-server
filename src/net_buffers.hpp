#pragma once

#include <enet/enet.h>
//
#include <lux/common.hpp>
#include <lux/net/server/tick.hpp>
#include <lux/net/server/signal.hpp>
#include <lux/net/server/init.hpp>
#include <lux/net/client/tick.hpp>
#include <lux/net/client/signal.hpp>
#include <lux/net/client/init.hpp>
#include <lux/net/serializer.hpp>
#include <lux/net/deserializer.hpp>

struct NetBuffers
{
    NetBuffers();
    ~NetBuffers();
    LUX_NO_COPY(NetBuffers);

    net::server::Tick   st;
    net::server::Signal ss;
    net::server::Init   si;
    net::client::Tick   ct;
    net::client::Signal cs;
    net::client::Init   ci;
    net::Serializer     serializer;
    net::Deserializer   deserializer;

    ENetPacket *unreliable_packet;
    ENetPacket *reliable_packet;
};
