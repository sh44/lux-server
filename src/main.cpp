#include <iostream>
//
#include <server.hpp>
//
#include <lua.hpp>
#include <data/lua_engine.hpp>

int main()
{
    Server server(30337, 128.0);
    server.start();
    std::string input;
    while(input != "stop")
    {
        std::cin >> input;
    }
    server.stop();
}
