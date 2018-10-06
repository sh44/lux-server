#include <map.hpp>
#include <server.hpp>
#include "api.hpp"

extern "C" {

void quit() {
    server_quit();
}

void make_admin(char const* name) {
    if(server_make_admin(name) != LUX_OK) {
        LUX_LOG("failed to make player \"%s\" an admin", name);
    }
}

void kick(char const* name, char const* reason) {
    kick_client(name, reason);
}

void broadcast(char const* str) {
    server_broadcast(str);
}

void place_light(ApiI64 x, ApiI64 y, ApiI64 z, ApiU8 r, ApiU8 g, ApiU8 b) {
    add_light_source({x, y, z}, {r, g, b});
}

}
