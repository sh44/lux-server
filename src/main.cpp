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
#include <lux_shared/net/net_order.hpp>
#include <lux_shared/net/data.hpp>
#include <lux_shared/net/enet.hpp>
#include <lux_shared/net/serial.hpp>
#include <lux_shared/util/tick_clock.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>

Uns constexpr MAX_CLIENTS  = 16;

struct {
    U16                 tick_rate = 64;
    Arr<U8, SERVER_NAME_LEN> name = {0};
} conf;

struct Server {
    struct Client {
        ENetPeer* peer;
        String    name;
        Entity*   entity;
        VecSet<ChkPos> loaded_chunks;
    };
    DynArr<Client> clients;

    std::atomic<bool> running = false;
    std::thread       thread;

    ENetHost*  host;
} server;

bool is_client_connected(Uns id) {
    return id < server.clients.size();
}

void erase_client(Uns id) {
    LUX_ASSERT(is_client_connected(id));
    LUX_LOG("client disconnected");
    LUX_LOG("    id: %zu" , id);
    LUX_LOG("    name: %s", server.clients[id].name.c_str());
    remove_entity(*server.clients[id].entity);
    server.clients.erase(server.clients.begin() + id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("terminating connection with peer");
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    enet_peer_disconnect_now(peer, 0);
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

LUX_MAY_FAIL add_client(ENetPeer* peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("new client connecting")
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    ENetPacket* in_pack;

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
            in_pack = enet_peer_receive(peer, &channel_id);
            if(in_pack != nullptr) {
                if(channel_id == INIT_CHANNEL) {
                    break;
                } else {
                    LUX_LOG("ignoring unexpected packet");
                    LUX_LOG("    channel: %u", channel_id);
                    enet_packet_destroy(in_pack);
                }
            }
            if(tries >= MAX_TRIES) {
                LUX_LOG("client did not send an init packet");
                return LUX_FAIL;
            }
            ++tries;
        } while(true);
        LUX_LOG("received init packet after %zu/%zu tries", tries, MAX_TRIES);
    }
    ///we need to keep the packet around, because we read its contents directly
    ///through the NetCsInit struct pointer
    LUX_DEFER { enet_packet_destroy(in_pack); };

    NetCsInit cs_init;
    { ///parse client init packet
        U8 const* iter = in_pack->data;
        if(check_pack_size(sizeof(NetCsInit), iter, in_pack) != LUX_OK) {
            return LUX_FAIL;
        }

        deserialize(&iter, &cs_init.net_ver.major);
        deserialize(&iter, &cs_init.net_ver.minor);
        deserialize(&iter, &cs_init.net_ver.patch);
        deserialize(&iter, &cs_init.name);
        LUX_ASSERT(iter == in_pack->data + in_pack->dataLength);

        if(cs_init.net_ver.major != NET_VERSION_MAJOR) {
            LUX_LOG("client uses an incompatible major lux net api version");
            LUX_LOG("    ours: %u", NET_VERSION_MAJOR);
            LUX_LOG("    theirs: %u", cs_init.net_ver.major);
            return LUX_FAIL;
        }
        if(cs_init.net_ver.minor >  NET_VERSION_MINOR) {
            LUX_LOG("client uses a newer minor lux net api version");
            LUX_LOG("    ours: %u", NET_VERSION_MINOR);
            LUX_LOG("    theirs: %u", cs_init.net_ver.minor);
            return LUX_FAIL;
        }
    }

    { ///send init packet
        ENetPacket* out_pack;
        if(create_reliable_pack(out_pack, sizeof(NetSsInit)) != LUX_OK) {
            return LUX_FAIL;
        }
        U8* iter = out_pack->data;
        serialize(&iter, conf.name);
        serialize(&iter, conf.tick_rate);
        LUX_ASSERT(iter == out_pack->data + out_pack->dataLength);

        if(send_packet(peer, out_pack, INIT_CHANNEL) != LUX_OK) return LUX_FAIL;
    }

    Server::Client& client = server.clients.emplace_back();
    client.peer = peer;
    client.peer->data = (void*)(server.clients.size() - 1);
    client.name = String((char const*)cs_init.name);
    client.entity = &create_player();

    LUX_LOG("client connected successfully");
    LUX_LOG("    name: %s", client.name.c_str());
    return LUX_OK;
}

LUX_MAY_FAIL send_map_load(ENetPeer* peer, Slice<ChkPos> const& requests) {
    typedef NetSsSgnl::MapLoad::Chunk NetChunk;
    SizeT pack_sz = sizeof(NetSsSgnl::Header) + sizeof(NetSsSgnl::MapLoad) +
        requests.len * sizeof(NetChunk);

    ENetPacket* out_pack;
    if(create_reliable_pack(out_pack, pack_sz) != LUX_OK) {
        return LUX_FAIL;
    }

    U8* iter = out_pack->data;
    serialize(&iter, (U8 const&)NetSsSgnl::MAP_LOAD);
    serialize(&iter, (U32 const&)requests.len);
    for(Uns i = 0; i < requests.len; ++i) {
        guarantee_chunk(requests[i]);
        Chunk const& chunk = get_chunk(requests[i]);

        serialize(&iter, requests[i]);
        serialize(&iter, chunk.voxels);
        serialize(&iter, chunk.light_lvls);
    }
    LUX_ASSERT(iter == out_pack->data + out_pack->dataLength);
    if(send_packet(peer, out_pack, SIGNAL_CHANNEL) != LUX_OK) return LUX_FAIL;

    ///we need to do it outside, because we must be sure that the packet has
    ///been received (i.e. sent in this case, enet guarantees that the peer will
    ///either receive it or get disconnected
    Server::Client& client = server.clients[(Uns)peer->data];
    for(Uns i = 0; i < requests.len; ++i) {
        client.loaded_chunks.emplace(requests[i]);
    }
    return LUX_OK;
}

//@TODO use slice
LUX_MAY_FAIL send_light_update(ENetPeer* peer, DynArr<ChkPos> const& updates) {
    Server::Client& client = server.clients[(Uns)peer->data];
    SizeT output_len = 0;
    for(Uns i = 0; i < updates.size(); ++i) {
        ChkPos const& pos = updates[i];
        if(client.loaded_chunks.count(pos) > 0) {
            ++output_len;
        }
    }
    typedef NetSsSgnl::LightUpdate::Chunk NetChunk;
    SizeT pack_sz = sizeof(NetSsSgnl::Header) + sizeof(NetSsSgnl::LightUpdate) +
        output_len * sizeof(NetChunk);

    ENetPacket* out_pack;
    if(create_reliable_pack(out_pack, pack_sz) != LUX_OK) {
        return LUX_FAIL;
    }

    U8* iter = out_pack->data;
    serialize(&iter, (U8 const&)NetSsSgnl::LIGHT_UPDATE);
    serialize(&iter, (U32 const&)output_len);
    for(Uns i = 0; i < updates.size(); ++i) {
        ChkPos const& pos = updates[i];
        if(client.loaded_chunks.count(pos) > 0) {
            Chunk const& chunk = get_chunk(pos);
            serialize(&iter, pos);
            serialize(&iter, chunk.light_lvls);
        }
    }
    LUX_ASSERT(iter == out_pack->data + out_pack->dataLength);
    if(send_packet(peer, out_pack, SIGNAL_CHANNEL) != LUX_OK) return LUX_FAIL;
    return LUX_OK;
}

LUX_MAY_FAIL handle_tick(ENetPeer* peer, ENetPacket *in_pack) {
    U8 const* iter = in_pack->data;
    if(check_pack_size(sizeof(NetCsTick), iter, in_pack) != LUX_OK) {
        return LUX_FAIL;
    }

    LUX_ASSERT(is_client_connected((Uns)peer->data));
    Entity& entity = *server.clients[(Uns)peer->data].entity;
    Vec2F player_dir;
    deserialize(&iter, &player_dir);
    LUX_ASSERT(iter == in_pack->data + in_pack->dataLength);
    if(player_dir.x != 0.f || player_dir.y != 0.f) {
        player_dir = glm::normalize(player_dir);
        entity.vel.x = player_dir.x * 0.1f;
        entity.vel.y = player_dir.y * 0.1f;
    }
    return LUX_OK;
}

LUX_MAY_FAIL handle_signal(ENetPeer* peer, ENetPacket* in_pack) {
    U8 const* iter = in_pack->data;
    if(check_pack_size_atleast(sizeof(NetCsSgnl::Header), iter, in_pack)
           != LUX_OK) {
        LUX_LOG("couldn't read signal header");
        return LUX_FAIL;
    }

    NetCsSgnl sgnl;
    deserialize(&iter, (U8*)&sgnl.header);

    if(sgnl.header >= NetCsSgnl::HEADER_MAX) {
        LUX_LOG("unexpected signal header %u", sgnl.header);
        return LUX_FAIL;
    }

    SizeT expected_stt_sz;
    switch(sgnl.header) {
        case NetCsSgnl::MAP_REQUEST: {
            expected_stt_sz = sizeof(NetCsSgnl::MapRequest);
        } break;
        default: LUX_UNREACHABLE();
    }
    if(check_pack_size_atleast(expected_stt_sz, iter, in_pack) != LUX_OK) {
        LUX_LOG("couldn't read static segment");
        return LUX_FAIL;
    }

    SizeT expected_dyn_sz;
    switch(sgnl.header) {
        case NetCsSgnl::MAP_REQUEST: {
            deserialize(&iter, &sgnl.map_request.requests.len);
            expected_dyn_sz = sgnl.map_request.requests.len * sizeof(ChkPos);
        } break;
        default: LUX_UNREACHABLE();
    }
    if(check_pack_size_atleast(expected_dyn_sz, iter, in_pack) != LUX_OK) {
        LUX_LOG("couldn't read dynamic segment");
        return LUX_FAIL;
    }

    switch(sgnl.header) {
        case NetCsSgnl::MAP_REQUEST: {
            Slice<ChkPos> requests;
            requests.len = sgnl.map_request.requests.len;
            requests.beg = lux_alloc<ChkPos>(requests.len);
            LUX_DEFER { lux_free(requests.beg); };

            deserialize(&iter, &requests.beg, requests.len);

            //@CONSIDER, should we really fail here? perhaps split the func
            if(send_map_load(peer, requests) != LUX_OK) return LUX_FAIL;
        } break;
        default: LUX_UNREACHABLE();
    }
    return LUX_OK;
}

void server_tick() {
    //@CONSIDER moving to server_main
    entities_tick();
    ///update lightning and send light updates
    {   static DynArr<ChkPos> light_updated_chunks;
        map_tick(light_updated_chunks);

        if(light_updated_chunks.size() > 0) {
            for(Server::Client& client : server.clients) {
                (void)send_light_update(client.peer, light_updated_chunks);
            }
            light_updated_chunks.clear();
        }
    }

    { ///handle events
        //@RESEARCH can we use our own packet to prevent copies?
        //@CONSIDER splitting this scope
        ENetEvent event;
        while(enet_host_service(server.host, &event, 0) > 0) {
            if(event.type == ENET_EVENT_TYPE_CONNECT) {
                if(add_client(event.peer) != LUX_OK) {
                    kick_peer(event.peer);
                }
            } else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                erase_client((Uns)event.peer->data);
            } else if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                LUX_DEFER { enet_packet_destroy(event.packet); };
                if(!is_client_connected((Uns)event.peer->data)) {
                    U8 *ip = get_ip(event.peer->address);
                    LUX_LOG("ignoring packet from not connected peer");
                    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                    enet_peer_reset(event.peer);
                } else {
                    if(event.channelID == TICK_CHANNEL) {
                        if(handle_tick(event.peer, event.packet) != LUX_OK) {
                            continue;
                        }
                    } else if(event.channelID == SIGNAL_CHANNEL) {
                        if(handle_signal(event.peer, event.packet) != LUX_OK) {
                            continue;
                        }
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

    { ///dispatch ticks
        for(Server::Client& client : server.clients) {
            ENetPacket* out_pack;
            if(create_unreliable_pack(out_pack, sizeof(NetSsTick)) != LUX_OK) {
                continue;
            }
            U8* iter = out_pack->data;
            serialize(&iter, client.entity->pos);
            LUX_ASSERT(iter == out_pack->data + out_pack->dataLength);
            (void)send_packet(client.peer, out_pack, TICK_CHANNEL);
        }
    }
}

void server_main(int argc, char** argv) {
    ///if we exit with an error
    LUX_DEFER { server.running = false; };

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

    db_init();
    LUX_LOG("initializing server");
    if(enet_initialize() != 0) {
        LUX_FATAL("couldn't initialize ENet");
    }
    LUX_DEFER { enet_deinitialize(); };

    {
        ENetAddress addr = {ENET_HOST_ANY, server_port};
        server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
        if(server.host == nullptr) {
            LUX_FATAL("couldn't initialize ENet host");
        }
    }
    LUX_DEFER { enet_host_destroy(server.host); };

    {
        U8 constexpr server_name[] = "lux-server";
        static_assert(sizeof(server_name) <= SERVER_NAME_LEN);
        std::memcpy(conf.name, server_name, sizeof(server_name));
    }

    { ///main loop
        auto tick_len = util::TickClock::Duration(1.0 / (F64)conf.tick_rate);
        util::TickClock clock(tick_len);
        while(server.running) {
            clock.start();
            server_tick();
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
    while(input != "stop" && server.running) {
        std::getline(std::cin, input);
    }

    LUX_LOG("stopping server");
    server.running = false;
    server.thread.join();
    return 0;
}
