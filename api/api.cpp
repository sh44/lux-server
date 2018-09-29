#include <server.hpp>

extern "C" {

void quit() {
    server_quit();
}

}
