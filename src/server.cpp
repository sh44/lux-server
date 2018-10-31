#include <cstring>
#include <algorithm>
//
#include <enet/enet.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/rotate_vector.hpp> //@TODO
//
#include <lux_shared/common.hpp>
#include <lux_shared/net/common.hpp>
#include <lux_shared/net/serial.hpp>
#include <lux_shared/net/data.hpp>
#include <lux_shared/net/data.inl>
#include <lux_shared/net/enet.hpp>
//
#include <map.hpp>
#include <entity.hpp>
#include <command.hpp>
#include "server.hpp"

Uns constexpr MAX_CLIENTS  = 16;

struct Server {
    F64 tick_rate = 0.0;
    struct Client {
        ENetPeer*      peer;
        DynStr         name;
        EntityId       entity;
        VecSet<ChkPos> loaded_chunks;
        bool           admin = false;
    };
    SparseDynArr<Client> clients;

    bool is_running = false;

    ENetHost*  host;
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
    server.is_running = true;
}

void server_deinit() {
    server.is_running = false;
    LUX_LOG("deinitializing server");

    LUX_LOG("kicking all clients");
    server.clients.foreach([](ClientId id) {
        kick_client(id, "server stopping");
    });
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
    LUX_LOG("    name: %s", server.clients[id].name.c_str());
    remove_entity(server.clients[id].entity);
    server.clients.erase(id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("terminating connection with peer");
    LUX_LOG("    ip: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    enet_peer_disconnect_now(peer, 0);
}

LUX_MAY_FAIL static server_send_msg(ClientId id, char const* beg, SizeT len) {
    char constexpr prefix[] = "[SERVER]: ";
    SizeT total_len = (sizeof(prefix) - 1) + len;
    ss_sgnl.tag = NetSsSgnl::MSG;
    ss_sgnl.msg.contents.resize(total_len);
    std::memcpy(ss_sgnl.msg.contents.data(), prefix, sizeof(prefix) - 1);
    std::memcpy(ss_sgnl.msg.contents.data() + (sizeof(prefix) - 1), beg, len);
    LUX_ASSERT(is_client_connected(id));
    return send_net_data(server.clients[id].peer, &ss_sgnl, SIGNAL_CHANNEL);
}

void kick_client(ClientId id, char const* reason) {
    LUX_ASSERT(is_client_connected(id));
    DynStr name = server.clients[id].name;
    LUX_LOG("kicking client");
    LUX_LOG("    name: %s", name.c_str());
    LUX_LOG("    reason: %s", reason);
    LUX_LOG("    id: %zu", id);
    DynStr msg = DynStr("you got kicked for: ") + DynStr(reason);
    (void)server_send_msg(id, msg.c_str(), msg.size());
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

        ClientId duplicate_id = get_client_id((const char*)cs_init.name);
        if(duplicate_id != server.clients.end()) {
            //@CONSIDER kicking the new one instead
            LUX_LOG("client already connected, kicking the old one");
            kick_client(duplicate_id, "double-join");
        }
    }

    { ///send init packet
        U8 constexpr server_name[] = "lux-server";
        static_assert(sizeof(server_name) <= SERVER_NAME_LEN);
        std::memcpy(ss_init.name, server_name, sizeof(server_name));
        std::memset(ss_init.name + sizeof(server_name), 0,
                    SERVER_NAME_LEN - sizeof(server_name));
        ss_init.tick_rate = server.tick_rate;

        LUX_RETHROW(send_net_data(peer, &ss_init, INIT_CHANNEL),
            "failed to send init data to client");
    }

    ClientId id = server.clients.emplace();
    Server::Client& client = server.clients[id];
    client.peer = peer;
    client.peer->data = (void*)id;
    client.name = DynStr((char const*)cs_init.name,
        strnlen((char const*)cs_init.name, CLIENT_NAME_LEN));
    client.entity = create_player();
    //@TODO
    entity_comps.shape[client.entity] = EntityComps::Shape
        {{.rect = {{0.8f, 0.4f}}}, EntityComps::Shape::RECT};
    entity_comps.container[client.entity].items =
        {create_item("dong"), create_item("bong"), create_item("levi's head")};
    entity_comps.visible[client.entity] = {0};
    auto& entity_name = entity_comps.name[client.entity];
    entity_name.resize(client.name.size());
    std::memcpy(entity_name.data(), client.name.data(), client.name.size());

    LUX_LOG("client connected successfully");
    LUX_LOG("    name: %s", client.name.c_str());
#ifndef NDEBUG
    (void)server_make_admin(id);
#endif
    return LUX_OK;
}

LUX_MAY_FAIL send_tiles(ENetPeer* peer, VecSet<ChkPos> const& requests) {
    ss_sgnl.tag = NetSsSgnl::TILES;
    for(auto const& pos : requests) {
        guarantee_chunk(pos);
        Chunk const& chunk = get_chunk(pos);
        for(Uns i = 0; i < CHK_VOL; ++i) {
            ss_sgnl.tiles.chunks[pos].id[i] =
                chunk.wall[i] ? chunk.fg_id[i] : chunk.bg_id[i];
        }
        std::memcpy(ss_sgnl.tiles.chunks[pos].wall.raw_data,
                    chunk.wall.raw_data, sizeof(chunk.wall));
    }

    LUX_RETHROW(send_net_data(peer, &ss_sgnl, SIGNAL_CHANNEL),
        "failed to send map load data to client");

    ///we need to do it outside, because we must be sure that the packet has
    ///been received (i.e. sent in this case, enet guarantees that the peer will
    ///either receive it or get disconnected
    Server::Client& client = server.clients[(ClientId)peer->data];
    for(auto const& pos : requests) {
        client.loaded_chunks.emplace(pos);
    }
    return LUX_OK;
}

LUX_MAY_FAIL send_light_update(ENetPeer* peer, DynArr<ChkPos> const& updates) {
    ss_sgnl.tag = NetSsSgnl::LIGHT;

    Server::Client& client = server.clients[(ClientId)peer->data];
    for(Uns i = 0; i < updates.size(); ++i) {
        ChkPos const& pos = updates[i];
        if(client.loaded_chunks.count(pos) > 0) {
            Chunk const& chunk = get_chunk(pos);
            std::memcpy(ss_sgnl.light.chunks[pos].light_lvl,
                        chunk.light_lvl, CHK_VOL * sizeof(LightLvl));
        }
    }
    if(ss_sgnl.light.chunks.size() == 0) return LUX_OK;
    return send_net_data(peer, &ss_sgnl, SIGNAL_CHANNEL);
}

LUX_MAY_FAIL handle_tick(ENetPeer* peer, ENetPacket *in_pack) {
    LUX_ASSERT(is_client_connected((ClientId)peer->data));
    LUX_RETHROW(deserialize_packet(in_pack, &cs_tick),
        "failed to deserialize tick from client")

    EntityId entity = server.clients[(ClientId)peer->data].entity;
    if(entity_comps.vel.count(entity) > 0) {
        if(f32_cmp(glm::length(cs_tick.player_dir), 1.f)) {
            entity_comps.vel.at(entity).x = cs_tick.player_dir.x * 0.1f;
            entity_comps.vel.at(entity).y = cs_tick.player_dir.y * 0.1f;
        }
    }
    if(entity_comps.orientation.count(entity) > 0) {
        entity_rotate_to(entity, cs_tick.player_aim_angle);
    }
    return LUX_OK;
}

LUX_MAY_FAIL handle_signal(ENetPeer* peer, ENetPacket* in_pack) {
    NetCsSgnl sgnl;
    LUX_RETHROW(deserialize_packet(in_pack, &sgnl),
        "failed to deserialize signal from client");

    switch(sgnl.tag) {
        case NetCsSgnl::MAP_REQUEST: {
            (void)send_tiles(peer, sgnl.map_request.requests);
        } break;
        case NetCsSgnl::COMMAND: {
            ClientId client_id = (ClientId)peer->data;
            if(!server.clients[client_id].admin) {
                LUX_LOG("client %s tried to execute command \"%*s\""
                        " without admin rights",
                        server.clients[client_id].name.c_str(),
                        (int)sgnl.command.contents.size(),
                        sgnl.command.contents.data());
                char constexpr DENY_MSG[] = "you do not have admin rights, this"
                    " incident will be reported";
                (void)server_send_msg(client_id, DENY_MSG,
                                sizeof(DENY_MSG));
                return LUX_FAIL;
            }
            LUX_LOG("[%s]: %s", server.clients[client_id].name.c_str(),
                    sgnl.command.contents.data());

            //@TODO we should redirect output somehow
            add_command(sgnl.command.contents.data());
        } break;
        default: LUX_UNREACHABLE();
    }
    return LUX_OK;
}

void server_tick(DynArr<ChkPos> const& light_updated_chunks) {
    server.clients.foreach([&](ClientId id) {
        (void)send_light_update(server.clients[id].peer, light_updated_chunks);
    });
    { ///handle events
        //@CONSIDER splitting this scope
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
                    } else if(event.channelID == SIGNAL_CHANNEL) {
                        if(handle_signal(event.peer, event.packet) != LUX_OK) {
                            LUX_LOG("failed to handle signal from client");
                            kick_client((ClientId)event.peer->data,
                                        "corrupted signal packet");
                            continue;
                        }
                    } else {
                        auto const &name =
                            server.clients[(ClientId)event.peer->data].name;
                        LUX_LOG("ignoring unexpected packet");
                        LUX_LOG("    channel: %u", event.channelID);
                        LUX_LOG("    from: %s", name.c_str());
                    }
                }
            }
        }
    }

    { ///dispatch ticks
        //@IMPROVE we might want to turn this into a differential transfer,
        //instead of resending the whole state all the time
        NetSsTick::EntityComps net_comps;
        get_net_entity_comps(&net_comps);
        ss_tick.entity_comps = net_comps;
        entities.foreach([&](EntityId id) {
            ss_tick.entities.emplace_back(id);
        });
        server.clients.foreach([&](ClientId id) {
            auto const& client = server.clients[id];
            ss_tick.player_id = client.entity;
            /*EntityId bullet = create_entity();
            entity_comps.vel[bullet] = glm::rotate(Vec2F(0.f, -1.f),
                entity_comps.orientation.at(client.entity).angle);
            entity_comps.pos[bullet] = entity_comps.pos.at(client.entity) +
                entity_comps.vel.at(bullet);
            entity_comps.shape[bullet] = EntityComps::Shape
                {{.sphere = {0.05f}}, .tag = EntityComps::Shape::SPHERE};
            entity_comps.visible[bullet] = {2};
            entity_comps.orientation[bullet] =
                {entity_comps.orientation.at(client.entity).angle};*/

            (void)send_net_data(client.peer, &ss_tick, TICK_CHANNEL, false);
        });
        clear_net_data(&ss_tick);
    }
    server.clients.free_slots();
}

void server_broadcast(char const* beg) {
    //@TODO use strlen, also should we really send the null terminator as well?
    //we send the length anyway
    char const* end = beg;
    while(*end != '\0') ++end;
    ///we count the null terminator
    ++end;
    SizeT len = end - beg;
    server.clients.foreach([&](ClientId id) {
        (void)server_send_msg(id, beg, len);
    });
}

bool server_is_running() {
    return server.is_running;
}

void server_quit() {
    server.is_running = false;
}

ClientId get_client_id(char const* name) {
    ClientId rval = server.clients.end();
    server.clients.foreach_while([&](ClientId id) {
        auto const& client = server.clients[id];
        if(std::strcmp(client.name.c_str(), name) == 0) {
            rval = id;
            return false;
        }
        return true;
    });
    return rval;
}

void server_make_admin(ClientId id) {
    LUX_LOG("making client %zu an admin", id);
    LUX_ASSERT(is_client_connected(id));
    server.clients[id].admin = true;
}
