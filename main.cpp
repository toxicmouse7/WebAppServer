#include <iostream>

#include "Server.hpp"

int main()
{
    Server server("127.0.0.1", 13372);
    server.run();
    return 0;
}
