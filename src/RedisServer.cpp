#include "RedisServer.h"
#include "RedisCommandHandler.h"
#include "RedisDatabase.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>

static RedisServer* globalServer = nullptr;

// Return the length in bytes of a complete RESP command at the start of buf.
// If incomplete or invalid, return 0 to indicate more data is needed.
static size_t respMessageLength(const std::string& buf) {
    if (buf.empty() || buf[0] != '*') return 0;
    size_t pos = 1;
    size_t crlf = buf.find("\r\n", pos);
    if (crlf == std::string::npos) return 0;
    int numElements = 0;
    try {
        numElements = std::stoi(buf.substr(pos, crlf - pos));
    } catch (...) { return 0; }
    pos = crlf + 2;
    for (int i = 0; i < numElements; ++i) {
        if (pos >= buf.size() || buf[pos] != '$') return 0;
        ++pos;
        crlf = buf.find("\r\n", pos);
        if (crlf == std::string::npos) return 0;
        int len = 0;
        try {
            len = std::stoi(buf.substr(pos, crlf - pos));
        } catch (...) { return 0; }
        pos = crlf + 2;
        if (pos + len + 2 > buf.size()) return 0; // need full bulk + CRLF
        pos += len + 2; // skip data + CRLF
    }
    return pos;
}

void signalHandler(int signum){
    if(globalServer){
        std::cout << "Caught signal " << signum << ", shutting down.\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler()
{
    signal(SIGINT, signalHandler);
}

RedisServer:: RedisServer(int port) : port(port), server_socket(-1), running(true) 
{
    globalServer = this;
    setupSignalHandler();
}

void RedisServer::shutdown()
{
    running = false;
    
    if(RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout <<"Database dumped to dump.my_rdb\n";
    }
    else {
        std::cerr << "Error dumping database. \n";
    }

    if (server_socket != INVALID_SOCK) {
        close(server_socket);
        server_socket = INVALID_SOCK;
    }

    std::cout << "Server Shutdown Complete" << "\n";
}

void RedisServer::run()
{
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        std::cerr << "Error creating server socket: " << std::strerror(errno) << "\n";
        return;
    }

    // // Best-effort: set close-on-exec
    int fdflags = fcntl(server_socket, F_GETFD);
    if (fdflags != -1) {
        if (fcntl(server_socket, F_SETFD, fdflags | FD_CLOEXEC) == -1) {
            std::cerr << "Warning: failed to set FD_CLOEXEC: " << std::strerror(errno) << "\n";
        }
    } else {
        std::cerr << "Warning: fcntl(F_GETFD) failed: " << std::strerror(errno) << "\n";
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno) << "\n";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket: " << std::strerror(errno) << "\n";
        close(server_socket);
        server_socket = INVALID_SOCK;
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error listening on server socket: " << std::strerror(errno) << "\n";
        close(server_socket);
        server_socket = INVALID_SOCK;
        return;
    }

    // // Non-blocking accept loop setup
    int nbflags = fcntl(server_socket, F_GETFL);
    if (nbflags != -1) {
        if (fcntl(server_socket, F_SETFL, nbflags | O_NONBLOCK) == -1) {
            std::cerr << "Warning: failed to set O_NONBLOCK: " << std::strerror(errno) << "\n";
        }
    } else {
        std::cerr << "Warning: fcntl(F_GETFL) failed: " << std::strerror(errno) << "\n";
    }

    std::cout << "Redis Server Listening on Port " << port << "\n";

    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    //alternate loop
    while(running){
        int client_socket = accept(server_socket, nullptr, nullptr);

        if (client_socket < 0) {
            if (!running) break;
            // Non-blocking accept: handle transient errors gracefully
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                struct pollfd pfd;
                pfd.fd = server_socket;
                pfd.events = POLLIN;
                pfd.revents = 0;
                (void)poll(&pfd, 1, 250); // wait a bit for next event
                continue;
            }
            if (errno == EMFILE || errno == ENFILE) {
                std::cerr << "accept() failed: FD limit reached: " << std::strerror(errno) << "\n";
                continue;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            continue;
        }

        threads.emplace_back([client_socket, &cmdHandler](){
            std::string inbuf;
            char buffer[1024];
            while (true) {
                int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytes <= 0) break; // connection closed or error
                inbuf.append(buffer, buffer + bytes);

                // Try to process as many complete commands as available
                while (true) {
                    if (inbuf.empty()) break;
                    size_t consume = 0;
                    if (inbuf[0] == '*') {
                        consume = respMessageLength(inbuf);
                        if (consume == 0) break; // need more data
                    } else {
                        size_t nl = inbuf.find('\n');
                        if (nl == std::string::npos) break; // wait for full line
                        consume = nl + 1;
                    }

                    std::string request = inbuf.substr(0, consume);
                    std::string response = cmdHandler.processCommand(request);
                    (void)send(client_socket, response.c_str(), response.size(), 0);
                    inbuf.erase(0, consume);
                }
            }
            close(client_socket);
        });
    }

    

    for(auto& t : threads){
        if(t.joinable()) t.join();
    }

    // beforeShutdown persist the database
    if(RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout <<"Database dumped to dump.my_rdb\n";
    }
    else {
        std::cerr << "Error dumping database. \n";
    }
    

    if (server_socket != INVALID_SOCK) {
        close(server_socket);
        server_socket = INVALID_SOCK;
    }
}

// accept loop with some error handling
    // while (running) {
    //     struct pollfd pfd;
    //     pfd.fd = server_socket;
    //     pfd.events = POLLIN;
    //     pfd.revents = 0;

    //     int pr = poll(&pfd, 1, 1000); // 1 second timeout
    //     if (pr < 0) {
    //         if (errno == EINTR) {
    //             continue; // interrupted by signal, retry
    //         }
    //         std::cerr << "poll() error: " << std::strerror(errno) << "\n";
    //         break;
    //     } else if (pr == 0) {
    //         continue; // timeout, loop to check running flag
    //     }

    //     if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    //         std::cerr << "poll() reported socket error/hangup\n";
    //         break;
    //     }

    //     if (pfd.revents & POLLIN) {
    //         sockaddr_in clientAddr{};
    //         socklen_t addrlen = sizeof(clientAddr);
    //         int client_fd = accept(server_socket, (struct sockaddr*)&clientAddr, &addrlen);
    //         if (client_fd < 0) {
    //             if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
    //                 continue; // transient, try again
    //             }
    //             if (errno == EMFILE || errno == ENFILE) {
    //                 std::cerr << "accept() failed: FD limit reached: " << std::strerror(errno) << "\n";
    //                 continue;
    //             }
    //             std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
    //             continue;
    //         }

    //         // Best-effort: set close-on-exec on client
    //         int cfdflags = fcntl(client_fd, F_GETFD);
    //         if (cfdflags != -1) {
    //             (void)fcntl(client_fd, F_SETFD, cfdflags | FD_CLOEXEC);
    //         }

    //         // For now, immediately close the client connection (skeleton)
    //         close(client_fd);
    //     }
    // }