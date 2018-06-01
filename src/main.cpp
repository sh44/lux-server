#include <iostream>
//
#include <net/enet_handle.hpp>
#include <server.hpp>
#include <data/lua_engine.hpp>

int main()
{
    net::ENetHandle enet_handle;
    Server server(31337, 128.0);
    server.start();
    std::string input;
    while(input != "stop")
    {
        std::cin >> input;
    }
    server.stop();
    return 0;
}
