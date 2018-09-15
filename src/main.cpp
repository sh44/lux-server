#include <algorithm>
#include <atomic>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <cstring>
//
#include <enet/enet.h>
//
#include <lux_shared/common.hpp>
#include <lux_shared/net/common.hpp>
#include <lux_shared/net/data.hpp>
#include <lux_shared/util/tick_clock.hpp>
//
#include <map.hpp>

Uns constexpr MAX_CLIENTS  = 16;

struct {
    U16                 tick_rate = 64;
    Arr<U8, SERVER_NAME_LEN> name = {0};
} conf;

struct Server {
    struct Client {
        ENetPeer* peer;
        String    name;
    };
    std::atomic<bool> running = false;
    std::thread       thread;

    ENetHost*     host;
    ENetPacket*   reliable_out;
    ENetPacket* unreliable_out;

    DynArr<Client>    clients;
} server;

bool is_client_connected(Uns id) {
    return id < server.clients.size();
}

void erase_client(Uns id) {
    LUX_ASSERT(is_client_connected(id));
    LUX_LOG("client disconnected");
    LUX_LOG("    id: %zu" , id);
    LUX_LOG("    name: %s", server.clients[id].name.c_str());
    //@TODO delete entity
    server.clients.erase(server.clients.begin() + id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("terminating connection with peer");
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    enet_peer_disconnect(peer, 0);
    enet_host_flush(server.host);
    enet_peer_reset(peer);
}

void kick_client(String const& name, String const& reason) {
    LUX_LOG("kicking client");
    LUX_LOG("    name: %s", name.c_str());
    LUX_LOG("    reason: %s", reason.c_str());
    auto it = std::find_if(server.clients.begin(), server.clients.end(),
        [&] (Server::Client const& v) { return v.name == name; });
    Uns client_id = it - server.clients.begin();
    LUX_LOG("    id: %zu", client_id);
    if(!is_client_connected(client_id)) {
        LUX_LOG("tried to kick non-existant client");
        return; //@CONSIDER return value for failure
    }
    //@TODO kick message
    kick_peer(server.clients[client_id].peer);
    erase_client(client_id);
}

//@CONSIDER custom error codes (enum?)
int add_client(ENetPeer* peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("new client connecting")
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    NetClientInit* client_init_data;
    ENetPacket*    init_pack;

    { ///retrieve init packet
        LUX_LOG("awaiting init packet");
        Uns constexpr MAX_TRIES = 10;
        Uns constexpr TRY_TIME  = 50; ///in milliseconds

        //@CONSIDER sleeping in a separate thread, so the server cannot be
        //frozen by malicious joining, perhaps a different, sleep-free solution
        //could be used
        Uns tries = 0;
        U8  channel_id;
        do {
            enet_host_service(server.host, nullptr, TRY_TIME);
            init_pack = enet_peer_receive(peer, &channel_id);
            if(init_pack != nullptr) {
                if(channel_id == INIT_CHANNEL) {
                    break;
                } else {
                    LUX_LOG("ignoring unexpected packet");
                    LUX_LOG("    channel: %u", channel_id);
                    enet_packet_destroy(init_pack);
                }
            }
            if(tries >= MAX_TRIES) {
                LUX_LOG("client did not send an init packet");
                return -1;
            }
            ++tries;
        } while(true);
        LUX_LOG("received init packet");
        LUX_LOG("    %zu/%zu tries", tries, MAX_TRIES);
    }
    ///we need to keep the packet around, because we read its contents directly
    ///through the NetClientInit struct pointer
    defer { enet_packet_destroy(init_pack); };

    { ///parse client init packet
        if(sizeof(NetClientInit) != init_pack->dataLength) {
            LUX_LOG("client sent invalid init packet");
            LUX_LOG("    expected size: %zuB", sizeof(NetClientInit));
            LUX_LOG("    actual size: %zuB", init_pack->dataLength);
            return -1;
        }

        client_init_data = (NetClientInit*)init_pack->data;

        if(client_init_data->net_ver.major != NET_VERSION_MAJOR) {
            LUX_LOG("client uses an incompatible major lux net api version");
            LUX_LOG("    ours: %u", NET_VERSION_MAJOR);
            LUX_LOG("    theirs: %u", client_init_data->net_ver.major);
            return -1;
        }
        if(client_init_data->net_ver.minor >  NET_VERSION_MINOR) {
            LUX_LOG("client uses a newer minor lux net api version");
            LUX_LOG("    ours: %u", NET_VERSION_MINOR);
            LUX_LOG("    theirs: %u", client_init_data->net_ver.minor);
            return -1;
        }
    }

    { ///send init packet
        //@CONSIDER putting it outside function scope, as this struct gets
        //reused for every client connection
        NetServerInit server_init_data = {conf.name, net_order(conf.tick_rate)};

        server.reliable_out->data = (U8*)&server_init_data;
        server.reliable_out->dataLength = sizeof(server_init_data);
        if(enet_peer_send(peer, INIT_CHANNEL, server.reliable_out) < 0) {
            LUX_LOG("failed to send server init packet");
            return -1;
        }
        enet_host_flush(server.host);
    }

    Server::Client& client = server.clients.emplace_back();
    client.peer = peer;
    client.peer->data = (void*)(server.clients.size() - 1);
    client.name = String((char const*)client_init_data->name.data());
    //@TODO set entity

    LUX_LOG("client connected successfully");
    LUX_LOG("    name: %s", client.name.c_str());
    return 0;
}

//@CONSIDER using a slice
void send_signal(ENetPeer* peer, U8* data, SizeT len) {
    server.reliable_out->data       = data;
    server.reliable_out->dataLength = len;
    if(enet_peer_send(peer, SIGNAL_CHANNEL, server.reliable_out) < 0) {
        LUX_LOG("failed to send server channel packet");
        //@TODO some info here
    }
    enet_host_flush(server.host);
}

void handle_tick(ENetPeer* peer, ENetPacket *pack) {

}

void handle_signal(ENetPeer* peer, ENetPacket *pack) {
    auto log_fail = [&]() {
        U8 *ip = get_ip(peer->address);
        LUX_LOG("    ignoring packet");
        LUX_LOG("    size: %zuB", pack->dataLength);
        LUX_LOG("    peer: ");
        LUX_LOG("        ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        LUX_LOG("        id: %zu", (Uns)peer->data);
        LUX_LOG("        name: %s", server.clients[(Uns)peer->data].name.c_str());
    };
    if(pack->dataLength < 1) {
        //@TODO additional peer info
        LUX_LOG("couldn't read signal header, ignoring it");
        log_fail();
        return;
    }

    SizeT static_size;
    SizeT dynamic_size;
    Slice<U8> dynamic_segment;
    NetClientSignal* signal = (NetClientSignal*)pack->data;

    { ///verify size
        ///we don't count the header
        SizeT static_dynamic_size = pack->dataLength - 1;
        SizeT needed_static_size;
        switch(signal->type) {
            case NetClientSignal::MAP_REQUEST: {
                needed_static_size = sizeof(NetClientSignal::MapRequest);
            } break;
            default: {
                LUX_LOG("unexpected signal type, ignoring it");
                LUX_LOG("    type: %u", signal->type);
                log_fail();
                return;
            }
        }
        if(static_dynamic_size < needed_static_size) {
            LUX_LOG("received packet static segment too small");
            LUX_LOG("    expected size: atleast %zuB", needed_static_size + 1);
            log_fail();
            return;
        }
        static_size = needed_static_size;
        dynamic_size = static_dynamic_size - static_size;
        SizeT needed_dynamic_size;
        switch(signal->type) {
            case NetClientSignal::MAP_REQUEST: {
                needed_dynamic_size = signal->map_request.requests.len *
                                      sizeof(ChkPos);
            } break;
            default: LUX_ASSERT(false);
        }
        if(dynamic_size != needed_static_size) {
            LUX_LOG("received packet dynamic segment size differs from expected");
            LUX_LOG("    expected size: %zuB", needed_dynamic_size +
                                               static_size + 1);
            log_fail();
            return;
        }
        dynamic_segment.set((U8*)(pack->data + 1 + static_size), dynamic_size);
    }

    SizeT pack_len = 1 + static_size + dynamic_size;
    U8 *pack_data = new U8[pack_len];
    defer { delete[] pack_data; };

    { ///parse the packet
        switch(signal->type) {
            case NetClientSignal::MAP_REQUEST: {
                typedef NetServerSignal::MapLoad::Chunk NetChunk;
                Slice<ChkPos> requests = dynamic_segment;

                NetChunk* chunks = (NetChunk*)(pack_data + 1 + static_size);
                for(Uns i = 0; i < requests.len; ++i) {
                    requests[i].x = net_order(requests[i].x);
                    requests[i].y = net_order(requests[i].y);
                    requests[i].z = net_order(requests[i].z);

                    guarantee_chunk(requests[i]);
                    Chunk const& chunk = get_chunk(requests[i]);
                    chunks[i].pos = requests[i];
                    ///wish we could just cast the pointer to it, but we need to
                    ///change the net order after all...
                    for(Uns j = 0; j < CHK_VOL; ++j) {
                        chunks[i].voxels[j] = net_order(chunk.voxels[j]);
                        chunks[i].light_lvls[j] =
                            net_order(chunk.light_lvls[j]);
                    }
                }
            } break;
            default: LUX_ASSERT(false);
        }
    }

    send_signal(peer, pack_data, pack_len);
}

void do_tick() {
    //@RESEARCH can we use our own packet to prevent copies?
    { ///handle events
        ENetEvent event;
        while(enet_host_service(server.host, &event, 0) > 0) {
            if(event.type == ENET_EVENT_TYPE_CONNECT) {
                if(add_client(event.peer) < 0) {
                    kick_peer(event.peer);
                }
            } else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                erase_client((Uns)event.peer->data);
            } else if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                defer { enet_packet_destroy(event.packet); };
                if(!is_client_connected((Uns)event.peer->data)) {
                    U8 *ip = get_ip(event.peer->address);
                    LUX_LOG("ignoring packet from not connected peer");
                    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                } else {
                    if(event.channelID == TICK_CHANNEL) {
                        handle_tick(event.peer, event.packet);
                    } else if(event.channelID == SIGNAL_CHANNEL) {
                        handle_signal(event.peer, event.packet);
                    } else {
                        auto const &name =
                            server.clients[(Uns)event.peer->data].name;
                        LUX_LOG("ignoring unexpected packet");
                        LUX_LOG("    channel: %u", event.channelID);
                        LUX_LOG("    from: %s", name.c_str());
                    }
                }
            }
        }
    }

    { ///world tick

    }
}

void server_main(int argc, char** argv) {
    U16 server_port;

    { ///read commandline args
        if(argc != 2) {
            LUX_FATAL("usage: %s SERVER_PORT", argv[0]);
        }
        U64 raw_server_port = std::atol(argv[1]);
        if(raw_server_port >= 1 << 16) {
            LUX_FATAL("invalid port %zu given", raw_server_port);
        }
        server_port = raw_server_port;
    }

    LUX_LOG("initializing server");
    if(enet_initialize() != 0) {
        LUX_FATAL("couldn't initialize ENet");
    }
    defer { enet_deinitialize(); };

    {
        ENetAddress addr = {ENET_HOST_ANY, server_port};
        server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
        if(server.host == nullptr) {
            LUX_FATAL("couldn't initialize ENet host");
        }
    }
    defer { enet_host_destroy(server.host); };

    {
        server.reliable_out = enet_packet_create(nullptr, 0,
            ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_NO_ALLOCATE);
        if(server.reliable_out == nullptr) {
            LUX_FATAL("couldn't initialize reliable output packet");
        }
        server.unreliable_out = enet_packet_create(nullptr, 0,
            ENET_PACKET_FLAG_UNSEQUENCED | ENET_PACKET_FLAG_NO_ALLOCATE);
        if(server.unreliable_out == nullptr) {
            LUX_FATAL("couldn't initialize unreliable output packet");
        }
    }

    {
        U8 constexpr server_name[] = "lux-server";
        static_assert(sizeof(server_name) <= SERVER_NAME_LEN);
        std::memcpy(conf.name.data(), server_name, sizeof(server_name));
    }

    { ///main loop
        auto tick_len = util::TickClock::Duration(1.0 / (F64)conf.tick_rate);
        util::TickClock clock(tick_len);
        while(server.running) {
            clock.start();
            do_tick();
            clock.stop();
            auto remaining = clock.synchronize();
            if(remaining < util::TickClock::Duration::zero()) {
                LUX_LOG("tick overhead of %.2fs", std::abs(remaining.count()));
            }
        }
    }

    LUX_LOG("deinitializing server");

    { ///kick all
        LUX_LOG("kicking all clients");
        auto it = server.clients.begin();
        while(it != server.clients.end()) {
            kick_client(it->name, "server stopping");
            it = server.clients.begin();
        }
    }
}

int main(int argc, char** argv) {
    LUX_LOG("starting server");
    server.running = true;
    server.thread = std::thread(&server_main, argc, argv);

    std::string input;
    while(input != "stop") {
        std::getline(std::cin, input);
    }

    LUX_LOG("stopping server");
    server.running = false;
    server.thread.join();
    return 0;
}
