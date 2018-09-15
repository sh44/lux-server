#include <algorithm>
#include <atomic>
#include <thread>
#include <iostream>
#include <cstdlib>
//
#include <enet/enet.h>
//
#include <lux_shared/common.hpp>
#include <lux_shared/net/common.hpp>
#include <lux_shared/net/data.hpp>
#include <lux_shared/util/tick_clock.hpp>

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
    LUX_LOG("client %s disconnected", server.clients[id].name.c_str());
    //@TODO delete entity
    server.clients.erase(server.clients.begin() + id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("terminating connection with %u.%u.%u.%u",
            ip[0], ip[1], ip[2], ip[3]);
    enet_peer_disconnect(peer, 0);
    enet_host_flush(server.host);
    enet_peer_reset(peer);
}

void kick_client(String const& name, String const& reason) {
    LUX_LOG("kicking client %s, reason %s", name.c_str(), reason.c_str());
    auto it = std::find_if(server.clients.begin(), server.clients.end(),
        [&] (Server::Client const& v) { return v.name == name; });
    Uns client_id = it - server.clients.begin();
    if(!is_client_connected(client_id)) {
        LUX_LOG("tried to kick non-existant client %s", name.c_str());
        return; //@CONSIDER return value for failure
    }
    //@TODO kick message
    kick_peer(server.clients[client_id].peer);
    erase_client(client_id);
}

//@CONSIDER custom error codes (enum?)
int add_client(ENetPeer* peer) {
    U8* ip = get_ip(peer->address);
    LUX_LOG("new client %u.%u.%u.%u connecting", ip[0], ip[1], ip[2], ip[3]);

    NetClientInit client_init_data;

    { ///retrieve init packet
        LUX_LOG("awaiting init packet");
        Uns constexpr MAX_TRIES = 10;
        Uns constexpr TRY_TIME  = 50; ///in milliseconds

        //@CONSIDER sleeping in a separate thread, so the server cannot be
        //frozen by malicious joining, perhaps a different, sleep-free solution
        //could be used
        Uns tries = 0;
        U8  channel_id;
        ENetPacket* init_pack;
        do {
            enet_host_service(server.host, nullptr, TRY_TIME);
            init_pack = enet_peer_receive(peer, &channel_id);
            if(init_pack != nullptr && channel_id != INIT_CHANNEL) {
                LUX_LOG("ignoring unexpected packet on channel %u", channel_id);
                enet_packet_destroy(init_pack);
                init_pack = nullptr;
            }
            if(tries >= MAX_TRIES) {
                LUX_LOG("client did not send an init packet");
                return -1;
            }
            ++tries;
        } while(init_pack == nullptr);
        LUX_LOG("received init packet after %zu/%zu tries", tries, MAX_TRIES);

        if(sizeof(client_init_data) != init_pack->dataLength) {
            LUX_LOG("client sent invalid init packet with size %zu instead of"
                    " %zu", sizeof(client_init_data), init_pack->dataLength);
            return -1;
        }

        //@CONSIDER using a generic de/serializer function for this
        std::memcpy((U8*)&client_init_data, init_pack->data,
                    sizeof(client_init_data));
        enet_packet_destroy(init_pack);

        if(client_init_data.net_ver.major != NET_VERSION_MAJOR) {
            LUX_LOG("client uses an incompatible major lux net api version"
                    ", we use %u, they use %u",
                    NET_VERSION_MAJOR, client_init_data.net_ver.major);
            return -1;
        }
        if(client_init_data.net_ver.minor >  NET_VERSION_MINOR) {
            LUX_LOG("client uses a newer minor lux net api version"
                    ", we use %u, they use %u",
                    NET_VERSION_MINOR, client_init_data.net_ver.minor);
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

    Server::Client* client = &server.clients.emplace_back();
    client->peer = peer;
    client->peer->data = (void*)(server.clients.size() - 1);
    client->name = String((char const*)client_init_data.name.data());

    LUX_LOG("client %s connected successfully", client->name.c_str());
    //@TODO set entity
    return 0;
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
                enet_packet_destroy(event.packet);
            }
        }
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

    { ///init server
        ENetAddress addr = {ENET_HOST_ANY, server_port};
        server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
        if(server.host == nullptr) {
            LUX_FATAL("couldn't initialize ENet host");
        }
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

    enet_host_destroy(server.host);
    enet_deinitialize();
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
