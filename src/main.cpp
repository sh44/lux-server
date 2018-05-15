#include <ncurses.h>
//
#include <server.hpp>

int main()
{
    initscr();
    Server server(30337, 128.0);
    server.start();
    std::string input;
    while(input != "stop")
    {
        char buffer[0x100];
        getstr(buffer);
        input = buffer;
    }
    server.stop();
    endwin();
}
