#include <iostream>
#include "server.h"

int main() {

    Server *server = Server::GetInstance();
    server->Start();
    return 0;
}