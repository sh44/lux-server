#include <algorithm>
#include <atomic>
#include <thread>
#include <iostream>
//
#include <enet/enet.h>
//
#include <common.hpp>
#include <shared.hpp>

Uns constexpr MAX_CLIENTS  = 16;

struct {
    U16 port = 31337;
    Arr<U8, SERVER_NAME_LEN> name = {0};
} conf;

};

struct Server {
    struct Client {
        ENetPeer* peer;
        String    name;
    };
    std::atomic<bool> running = false;
    std::thread       thread;
    ENetHost*         host;
    DynArr<Client>    clients;
} server;

void erase_client(Uns id) {
    LUX_ASSERT(id < server.clients.size());
    LUX_LOG("client %s disconnected", server.clients[id].name.c_str());
    //@TODO delete entity
    server.clients.erase(server.clients.begin() + id);
}

void kick_peer(ENetPeer *peer) {
    U8* ip = (U8*)&peer->address.host;
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
    if(client_id >= server.clients.size()) {
        LUX_LOG("tried to kick non-existant client %s", name.c_str());
        return; //@CONSIDER return value for failure
    }
    //@TODO kick message
    kick_peer(server.clients[client_id].peer);
    erase_client(client_id);
}

int add_client(ENetPeer* peer) {
    U8* ip = (U8*)&peer->address.host;
    static_assert(sizeof(peer->address.host) == 4);
    LUX_LOG("new client %u.%u.%u.%u connecting", ip[0], ip[1], ip[2], ip[3]);

    ///retrieve init packet
    LUX_LOG("awaiting init packet");
    Uns                        constexpr MAX_TRIES = 10;
    std::chrono::duration<F64> constexpr TRY_TIME(0.050);

    //@CONSIDER sleeping in a separate thread, so the server cannot be frozen
    //by malicious joining, perhaps a different, sleep-free solution could be
    //used
    Uns tries = 0;
    U8  channel_id;
    ENetPacket* init_pack = nullptr;
    while(init_pack == nullptr) {
        init_pack = enet_peer_receive(peer, &channel_id);
        if(tries >= MAX_TRIES) {
            LUX_LOG("client did not send an init packet");
            return -1;
        }
        //@RESEARCH whether an unsequenced packet can arrive before a reliable
        //one, if so, we need to ignore tick packets here
        if(init_pack != nullptr && channel_id != INIT_CHANNEL) {
            LUX_LOG("client sent a packet on unexcepted channel %u"
                    ", we expected it on channel %u", channel_id, INIT_CHANNEL);
            return -1;
        }
        ++tries;
        std::this_thread::sleep_for(TRY_TIME);
    }
    LUX_LOG("received init packet after %zu", tries);

    ///we are gonna do a direct copy, so we disable padding,
    ///no need to reverse the byte order,
    ///because we are not using anything bigger than a byte right now,
    //@CONSIDER adding manual padding to increase efficiency
    #pragma pack(push, 1)
    struct
    {
        Arr<U8, 3> ver;
        Arr<U8, CLIENT_NAME_LEN> name;
    } client_init_data;
    #pragma pack(pop)

    if(sizeof(client_init_data) != init_pack->dataLength) {
        LUX_LOG("client sent invalid init packet with size %zu"
                "instead of %zu",
                sizeof(client_init_data), init_pack->dataLength);
        return -1;
    }
    ///remember, this assumes that there are no struct fields bigger
    ///than a byte, otherwise we would need to reverse the network order
    //@CONSIDER using a standarized deserializer function for this
    std::memcpy((U8*)&client_init_data, init_pack->data,
                sizeof(client_init_data));
    enet_packet_destroy(init_pack);

    static_assert(sizeof(client_init_data.ver[0]) ==
                  sizeof(LUX_NET_VERSION_MAJOR));
    static_assert(sizeof(client_init_data.ver[1]) ==
                  sizeof(LUX_NET_VERSION_MINOR));
    if(client_init_data.ver[0] != LUX_NET_VERSION_MAJOR) {
        LUX_LOG("client uses an incompatible major lux net api version"
                ", we use %u, they use %u",
                LUX_NET_VERSION_MAJOR, client_init_data.ver[0]);
        return -1;
    }
    if(client_init_data.ver[1] >  LUX_NET_VERSION_MINOR) {
        LUX_LOG("client uses a newer minor lux net api version"
                ", we use %u, they use %u",
                LUX_NET_VERSION_MINOR, client_init_data.ver[1]);
        return -1;
    }

    //@CONSIDER putting it outside function scope, as this struct gets
    //reused for every client connection, maybe even do it for the packet
    #pragma pack(push, 1)
    struct
    {
        Arr<U8, SERVER_NAME_LEN> name = conf.name;
    } server_init_data;
    #pragma pack(pop)

    ENetPacket* server_init_pack =
        enet_packet_create((U8*)&server_init_data,
        sizeof(server_init_data), ENET_PACKET_FLAG_RELIABLE);
    if(server_init_pack == nullptr) {
        LUX_LOG("failed to create server init packet");
        return -1;
    }
    if(enet_peer_send(peer, INIT_CHANNEL, server_init_pack) < 0) {
        LUX_LOG("failed to send server init packet");
        enet_packet_destroy(server_init_pack);
        return -1;
    }
    enet_host_flush(server.host);
    enet_packet_destroy(server_init_pack);

    Server::Client* client = &server.clients.emplace_back();
    client->peer = peer;
    client->peer->data = (void*)(server.clients.size() - 1);
    client->name = String((char const*)client_init_data.name.data());

    LUX_LOG("client %s connected successfully", client->name.c_str());
    //@TODO set entity
    return 0;
}

void server_main() {
    LUX_LOG("initializing server");
    if(enet_initialize() != 0) {
        LUX_FATAL("couldn't initialize ENet");
    }
    ENetAddress addr = {ENET_HOST_ANY, conf.port};
    server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
    if(server.host == nullptr) {
        LUX_FATAL("couldn't initialize ENet host");
    }
    U8 constexpr server_name[] = "lux-client";
    static_assert(sizeof(server_name) <= SERVER_NAME_LEN);
    std::memcpy(conf.name.data(), server_name, sizeof(server_name));

    ENetEvent event; //@RESEARCH can we use our own packet to prevent copies?
    while(server.running) {
        //@TODO world tick

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

    LUX_LOG("deinitializing server");
    LUX_LOG("kicking all clients");
    auto it = server.clients.begin();
    while(it != server.clients.end()) {
        kick_client(it->name, "server stopping");
        it = server.clients.begin();
    }
    enet_host_destroy(server.host);
    enet_deinitialize();
}

int main() {
    LUX_LOG("starting server");
    server.running = true;
    server.thread = std::thread(&server_main);

    std::string input;
    while(input != "stop") {
        std::getline(std::cin, input);
    }

    LUX_LOG("stopping server");
    server.running = false;
    server.thread.join();
    return 0;
}
