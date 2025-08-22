#include <atomic>
#include <string>
#include <iostream>


// #ifdef _WIN32
//     #include <winsock2.h>
//     #include <ws2tcpip.h>
//     #pragma comment(lib, "Ws2_32.lib")
//     #define CLOSESOCK closesocket
//     #include <BaseTsd.h>
//     typedef SSIZE_T ssize_t; 
//     using socket_t = SOCKET;
//     static const socket_t INVALID_SOCK = INVALID_SOCKET;
// #else 
//     #include <netdb.h>
//     #include <sys/socket.h>
//     #include <unistd.h>
//     #define CLOSESOCK close
//     using socket_t = int;
//     static const socket_t INVALID_SOCK = -1;
// #endif
    #include <netdb.h>
    #include <sys/socket.h>
    #include <unistd.h>
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
    static const int INVALID_SOCK = -1;
};

#endif