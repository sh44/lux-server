#include <iostream>
//
#include <lux/net/enet_handle.hpp>
//
#include <server.hpp>

int main()
{
    net::ENetHandle enet_handle;
    Server server(31337);
    std::string input;
    while(input != "stop")
    {
        std::cin >> input;
    }
    return 0;
}
