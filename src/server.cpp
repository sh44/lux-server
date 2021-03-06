#include <cstring>
#include <algorithm>
//
#include <enet/enet.h>
//
#include <lux_shared/common.hpp>
#include <lux_shared/net/common.hpp>
#include <lux_shared/net/data.hpp>
#include <lux_shared/net/data.inl>
#include <lux_shared/net/enet.hpp>
//
#include <map.hpp>
#include <entity.hpp>
#include "server.hpp"

Uns constexpr MAX_CLIENTS  = 16;

struct Server {
    F64 tick_rate = 0.0;
    struct Client {
        ENetPeer*      peer;
        StrBuff        name;
        EntityId       entity;
        VecSet<ChkPos> loaded_chunks;
        VecSet<ChkPos> pending_requests;
        bool           admin = false;
    };
    SparseDynArr<Client> clients;

    bool is_running = false;

    ENetHost* host;
} server;

NetCsSgnl cs_sgnl;
NetCsTick cs_tick;
NetSsInit ss_init;
NetSsSgnl ss_sgnl;
NetSsTick ss_tick;

void server_init(U16 port, F64 tick_rate) {
    server.tick_rate = tick_rate;

    LUX_LOG("initializing server");
    if(enet_initialize() != 0) {
        LUX_FATAL("couldn't initialize ENet");
    }

    {
        ENetAddress addr = {ENET_HOST_ANY, port};
        server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
        if(server.host == nullptr) {
            LUX_FATAL("couldn't initialize ENet host");
        }
    }
    net_compression_init(server.host);
    server.is_running = true;
}

static Server::Client& get_client(ENetPeer* peer) {
    return server.clients[(ClientId)peer->data];
}

void server_deinit() {
    server.is_running = false;
    LUX_LOG("deinitializing server");

    LUX_LOG("kicking all clients");
    for(auto pair : server.clients) {
        kick_client(pair.k, "server stopping"_l);
    }
    enet_host_destroy(server.host);
    enet_deinitialize();
}

bool is_client_connected(ClientId id) {
    return server.clients.contains(id);
}

void erase_client(ClientId id) {
    LUX_ASSERT(is_client_connected(id));
    LUX_LOG("client disconnected");
    LUX_LOG("    id: %zu" , id);
    auto const& name = server.clients[id].name;
    LUX_LOG("    name: %.*s", (int)name.len, name.beg);
    entity_erase(server.clients[id].entity);
    server.clients.erase(id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("terminating connection with peer");
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    enet_peer_disconnect_now(peer, 0);
}

LUX_MAY_FAIL static server_send_msg(ClientId id, Str str) {
    LUX_UNIMPLEMENTED();
    return LUX_OK;
}

void kick_client(ClientId id, Str reason) {
    LUX_ASSERT(is_client_connected(id));
    DynStr name = (DynStr)server.clients[id].name;
    LUX_LOG("kicking client");
    LUX_LOG("    name: %.*s", (int)name.len, name.beg);
    LUX_LOG("    reason: %.*s", (int)reason.len, reason.beg);
    LUX_LOG("    id: %zu", id);
    Str constexpr prefix = "you got kicked for: "_l;
    StrBuff msg(prefix.len + reason.len);
    msg.cpy(prefix).cpy(reason);
    (void)server_send_msg(id, msg);
    enet_host_flush(server.host);
    kick_peer(server.clients[id].peer);
    erase_client(id);
}

LUX_MAY_FAIL add_client(ENetPeer* peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("new client connecting");
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
        LUX_RETHROW(deserialize_packet(in_pack, &cs_init),
            "failed to deserialize init packet from client");

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

        ClientId duplicate_id = get_client_id((Str)cs_init.name);
        if(duplicate_id != server.clients.end().id) {
            //@CONSIDER kicking the new one instead
            LUX_LOG("client already connected, kicking the old one");
            kick_client(duplicate_id, "double-join"_l);
        }
    }

    ClientId id = server.clients.emplace();
    Server::Client& client = server.clients[id];
    client.peer = peer;
    client.peer->data = (void*)id;
    client.name = (Str)cs_init.name;
    client.entity = create_player();

    { ///send init packet
        Str constexpr name = "lux-server"_l;
        LUX_ASSERT(name.len <= SERVER_NAME_LEN);
        ((DynStr)ss_init.name).cpy(name).set('\0');
        ss_init.tick_rate    = server.tick_rate;
        if(send_net_data(peer, &ss_init, INIT_CHANNEL) != LUX_OK) {
            LUX_LOG_ERR("failed to send init data to client");
            entity_erase(client.entity);
            return LUX_FAIL;
        }
    }
    entity_comps.name[client.entity] = (Str)client.name;

    LUX_LOG("client connected successfully");
    LUX_LOG("    name: %.*s", (int)client.name.len, client.name.beg);
#ifndef NDEBUG
    (void)server_make_admin(id);
#endif
    return LUX_OK;
}

LUX_MAY_FAIL send_chunk_update(ENetPeer* peer, VecSet<ChkPos> const& chunks) {
    ss_sgnl.tag = NetSsSgnl::CHUNK_UPDATE;
    for(auto const& pos : chunks) {
        Chunk const& chunk = get_chunk(pos);
        LUX_ASSERT(chunk.mesh_state != Chunk::NOT_BUILT);
        auto const& mesh = *chunk.mesh;
        ss_sgnl.chunk_update.chunks[pos].removed_faces = mesh.removed_faces;
        ss_sgnl.chunk_update.chunks[pos].added_faces = mesh.added_faces;
    }

    LUX_RETHROW(send_net_data(peer, &ss_sgnl, SGNL_CHANNEL),
        "failed to send chunk updates to client");
    return LUX_OK;
}

LUX_MAY_FAIL handle_tick(ENetPeer* peer, ENetPacket *in_pack) {
    LUX_ASSERT(is_client_connected((ClientId)peer->data));
    LUX_RETHROW(deserialize_packet(in_pack, &cs_tick),
        "failed to deserialize tick from client")

    auto const& client = get_client(peer);
    if(cs_tick.is_moving) {
        F32 len = length(cs_tick.move_dir);
        if(len != 1.f && len != 0.f) {
            cs_tick.move_dir /= len;
        }
        entity_set_vel(client.entity, cs_tick.move_dir);
    } else {
        entity_set_vel(client.entity, Vec3F(0.f));
    }
    //entity_rotate_yaw(client.entity, cs_tick.yaw_pitch.x);
    entity_rotate_yaw_pitch(client.entity, cs_tick.yaw_pitch);
    return LUX_OK;
}

LUX_MAY_FAIL handle_signal(ENetPeer* peer, ENetPacket* in_pack) {
    NetCsSgnl sgnl;
    LUX_RETHROW(deserialize_packet(in_pack, &sgnl),
        "failed to deserialize signal from client");

    switch(sgnl.tag) {
        case NetCsSgnl::MAP_REQUEST: {
            Server::Client& client = get_client(peer);
            for(auto const& request : sgnl.map_request.requests) {
                try_guarantee_chunk_mesh(request);
                client.pending_requests.insert(request);
            }
        } break;
        default: LUX_UNREACHABLE();
    }
    return LUX_OK;
}

static void handle_pending_chunk_requests(Server::Client& client) {
    if(client.pending_requests.size() == 0) return;
    ss_sgnl.tag = NetSsSgnl::CHUNK_LOAD;

    static VecSet<ChkPos> loaded_chunks;
    loaded_chunks.clear();
    for(auto const& pos : client.pending_requests) {
        //@TODO we should really serialize this
        if(try_guarantee_chunk_mesh(pos)) {
            ChunkMesh& mesh = *get_chunk(pos).mesh;
            auto& net_chunk = ss_sgnl.chunk_load.chunks[pos];
            if(get_chunk(pos).mesh_state != Chunk::BUILT_EMPTY) {
                net_chunk.faces.resize(mesh.faces.len);
                for(Uns i = 0; i < mesh.faces.len; ++i) {
                    net_chunk.faces[i].idx         = mesh.faces[i].idx;
                    net_chunk.faces[i].id          = mesh.faces[i].id;
                    net_chunk.faces[i].orientation = mesh.faces[i].orientation;
                }
            }
            loaded_chunks.insert(pos);
        }
    }

    if(send_net_data(client.peer, &ss_sgnl, SGNL_CHANNEL) == LUX_OK) {
        //we want to be sure that the chunks have been sent,
        //so that there are no chunks that never load
        for(auto const& pos : loaded_chunks) {
            client.loaded_chunks.emplace(pos);
            client.pending_requests.erase(pos);
        }
    } else {
        LUX_LOG_ERR("failed to send map load data to client");
    }
}

void server_tick() {
    benchmark("1", 1.0 / 64.0, [&](){
    for(auto pair : server.clients) {
        static VecSet<ChkPos> chunks_to_send;
        for(auto const& pos : updated_meshes) {
            if(pair.v.loaded_chunks.count(pos) > 0) {
                chunks_to_send.insert(pos);
            }
        }
        if(chunks_to_send.size() > 0) {
            (void)send_chunk_update(pair.v.peer, chunks_to_send);
            chunks_to_send.clear();
        }
    }
    for(auto const& pos : updated_meshes) {
        auto const& chunk = get_chunk(pos);
        chunk.mesh->added_faces.clear();
        chunk.mesh->removed_faces.clear();
    }
    });
    updated_meshes.clear();
    benchmark("2", 1.0 / 64.0, [&](){
    { ///handle events
        ENetEvent event;
        while(enet_host_service(server.host, &event, 0) > 0) {
            if(event.type == ENET_EVENT_TYPE_CONNECT) {
                if(add_client(event.peer) != LUX_OK) {
                    kick_peer(event.peer);
                }
            } else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                erase_client((ClientId)event.peer->data);
            } else if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                LUX_DEFER { enet_packet_destroy(event.packet); };
                if(!is_client_connected((ClientId)event.peer->data)) {
                    U8 *ip = get_ip(event.peer->address);
                    LUX_LOG("ignoring packet from not connected peer");
                    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                    enet_peer_reset(event.peer);
                } else {
                    if(event.channelID == TICK_CHANNEL) {
                        if(handle_tick(event.peer, event.packet) != LUX_OK) {
                            continue;
                        }
                    } else if(event.channelID == SGNL_CHANNEL) {
                        if(handle_signal(event.peer, event.packet) != LUX_OK) {
                            LUX_LOG("failed to handle signal from client");
                            kick_client((ClientId)event.peer->data,
                                        "corrupted signal packet"_l);
                            continue;
                        }
                    } else {
                        auto const &name = get_client(event.peer).name;
                        LUX_LOG("ignoring unexpected packet");
                        LUX_LOG("    channel: %u", event.channelID);
                        LUX_LOG("    from: %.*s", (int)name.len, name.beg);
                    }
                }
            }
        }
    }
    });

    benchmark("3", 1.0 / 64.0, [&](){
    { ///dispatch ticks and other pending packets
        //@IMPROVE we might want to turn this into a differential transfer,
        //instead of resending the whole state all the time
        NetSsTick::EntityComps net_comps;
        get_net_entity_comps(&net_comps);
        ss_tick.day_cycle = day_cycle;
        ss_tick.entity_comps = net_comps;
        for(auto pair : entities) {
            ss_tick.entities.emplace(pair.k);
        }
        for(auto pair : server.clients) {
            auto& client = pair.v;
            ss_tick.player_id = client.entity;
            (void)send_net_data(client.peer, &ss_tick, TICK_CHANNEL, false);

            handle_pending_chunk_requests(client);
        }
        clear_net_data(&ss_tick);
    }
    server.clients.free_slots();
    });
}

void server_broadcast(Str msg) {
    for(auto pair : server.clients) {
        (void)server_send_msg(pair.k, msg);
    }
}

bool server_is_running() {
    return server.is_running;
}

void server_quit() {
    server.is_running = false;
}

ClientId get_client_id(Str name) {
    ClientId rval = server.clients.end().id;
    for(auto pair : server.clients) {
        auto const& client = pair.v;
        if((Str)client.name == name) {
            rval = pair.k;
            break;
        }
    }
    return rval;
}

void server_make_admin(ClientId id) {
    LUX_LOG("making client %zu an admin", id);
    LUX_ASSERT(is_client_connected(id));
    server.clients[id].admin = true;
}
