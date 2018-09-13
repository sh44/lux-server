#include <algorithm>
#include <atomic>
#include <thread>
#include <iostream>
//
#include <enet/enet.h>
//
#include <common.hpp>

Uns constexpr MAX_CLIENTS  = 16;
Uns constexpr CHANNEL_NUM = 3;

struct {
    U16 port;
} config;

struct Client {
    ENetPeer* peer;
    String    name;
};

struct {
    std::atomic<bool> running = false;
    std::thread       thread;
    ENetHost*         host;
    DynArr<Client>    clients;
} server;

void erase_client(Uns id) {
    LUX_ASSERT(id < server.clients.size());
    LUX_LOG("player %s disconnected", server.clients[id].name.c_str());
    //@TODO delete entity
    server.clients.erase(server.clients.begin() + id);
}

void kick_client(String const& name, String const& reason) {
    LUX_LOG("kicking player %s, reason %s", name.c_str(), reason.c_str());
    auto it = std::find_if(server.clients.begin(), server.clients.end(),
        [&] (Client const& v) { return v.name == name; });
    Uns client_id = it - server.clients.begin();
    if(client_id >= server.clients.size()) {
        LUX_LOG("tried to kick non-existant client %s", name.c_str());
        return; //@CONSIDER return value for failure
    }
    ENetPeer *peer = server.clients[client_id].peer;
    enet_peer_disconnect(peer, 0);
    enet_host_flush(server.host);
    //@TODO kick message
    enet_peer_reset(peer);
    erase_client(client_id);
}

void server_main() {
    LUX_LOG("initializing server");
    if(enet_initialize() != 0) {
        LUX_FATAL("couldn't initialize ENet");
    }
    ENetAddress addr = {ENET_HOST_ANY, config.port};
    server.host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_NUM, 0, 0);
    if(server.host == nullptr) {
        LUX_FATAL("couldn't initialize ENet host");
    }

    ENetEvent event;
    while(server.running) {
        //@TODO world tick

        while(enet_host_service(server.host, &event, 0) > 0) {
            if(event.type == ENET_EVENT_TYPE_CONNECT) {
                LUX_LOG("new client connected"); //@CONSIDER logging IP
                Client& client = server.clients.emplace_back();
                client.peer = event.peer;
                client.peer->data = (void *)(server.clients.size() - 1);
                //@TODO set entity
            } else if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                erase_client((Uns)event.peer->data);
            }
            //we ignore ENET_EVENT_TYPE_RECEIVE, because the Client handles it
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
