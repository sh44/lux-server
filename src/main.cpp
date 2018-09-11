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
        std::getline(std::cin, input);
        if(input == "stop") break;
        else if(input == "kick")
        {
            std::cout << "hostname: " << std::endl;
            std::getline(std::cin, input);

            ENetAddress addr;
            enet_address_set_host(&addr, input.c_str());

            std::cout << "reason: " << std::endl;
            std::getline(std::cin, input);
            server.kick_player(addr.host, input);
        }
    }
    return 0;
}
