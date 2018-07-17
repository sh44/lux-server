#include <iostream>
//
#include <lux/net/enet_handle.hpp>
//
#include <server.hpp>

int main()
{
    net::ENetHandle enet_handle;
    Server server(31337, 64.0);
    server.start();
    std::string input;
    while(input != "stop")
    {
        std::cin >> input;
    }
    server.stop();
    return 0;
}
