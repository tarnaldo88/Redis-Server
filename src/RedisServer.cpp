#include "./include/RedisServer.h"

static RedisServer* globalServer = nullptr;

RedisServer:: RedisServer(int port) : port(port), server_socket(-1), running(true) 
{
    globalServer = this;
}

void RedisServer::run()
{
}

void RedisServer::shutdown()
{
    running = false;

    if(server_socket != INVALID_SOCK) {
        close(server_socket);
    }

    std::cout << "Server Shutdown Complete";
}
