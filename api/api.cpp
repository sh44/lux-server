#include <server.hpp>

extern "C" {

void quit() {
    server_quit();
}

void make_admin(char const* name) {
    if(server_make_admin(name) != LUX_OK) {
        LUX_LOG("failed to make player \"%s\" an admin", name);
    }
}

}
