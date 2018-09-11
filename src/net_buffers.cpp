#include "net_buffers.hpp"

NetBuffers::NetBuffers() :
    unreliable_packet(enet_packet_create(nullptr, 0,
                                         ENET_PACKET_FLAG_UNSEQUENCED)),
    reliable_packet(enet_packet_create(nullptr, 0,
                                       ENET_PACKET_FLAG_RELIABLE))
{

}

NetBuffers::~NetBuffers()
{
    enet_packet_destroy(unreliable_packet);
    enet_packet_destroy(reliable_packet);
}
