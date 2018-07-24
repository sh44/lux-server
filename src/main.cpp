#include <iostream>
//
#include <enet/enet.h>
//
#include <lux/net/enet_handle.hpp>
//
#include <server.hpp>

int main()
{
    net::ENetHandle enet_handle;
    Server server(31337);
    std::string input;
    while(true)
    {
        std::cin >> input;
        if(input == "stop") break;
        else if(input == "kick")
        {
            std::cout << "hostname: ";
            std::cin >> input;

            ENetAddress addr;
            enet_address_set_host(&addr, input.c_str());
            addr.port = 31337;

            std::cout << "reason: ";
            std::cin >> input;
            server.kick_player(addr.host, input);
        }
    }
    return 0;
}
