#include <atomic>
#include <string>
#include <iostream>
#include <thread>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <vector>
#include <signal.h>

using socket_t = int;

#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

class RedisServer{
public:
    RedisServer(int port);
    void run();
    void shutdown();

private:
    int port;
    int server_socket;
    std::atomic<bool> running;
    static const socket_t INVALID_SOCK = -1;

    //setup signal handling for graceful shutdown
    void setupSignalHandler();
};

#endif