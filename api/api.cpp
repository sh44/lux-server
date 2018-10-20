#include <map.hpp>
#include <server.hpp>
#include <entity.hpp>
#include "api.hpp"

extern "C" {

void quit() {
    server_quit();
}

void make_admin(char const* name) {
    ClientId id;
    if(!get_client_id(&id, name)) {
        LUX_LOG("failed to make player \"%s\" an admin", name);
        return;
    }
    server_make_admin(id);
}

void kick(char const* name, char const* reason) {
    ClientId id;
    if(!get_client_id(&id, name)) {
        LUX_LOG("failed to kick player \"%s\"", name);
        return;
    }
    kick_client(id, reason);
}

void broadcast(char const* str) {
    server_broadcast(str);
}

void place_light(ApiI64 x, ApiI64 y, ApiI64 z, ApiU8 r, ApiU8 g, ApiU8 b) {
    add_light_source({x, y, z}, {r, g, b});
}

void new_entity(ApiF32 x, ApiF32 y, ApiF32 z) {
    LUX_UNIMPLEMENTED();
    /*
    EntityHandle id            = entities.emplace();
    entity_comps.pos[id]       = {x, y, z};
    entity_comps.vel[id]       = {0, 0, 0};
    entity_comps.shape[id].rad = 0.8;*/
}

}
