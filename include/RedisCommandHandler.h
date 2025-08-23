#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler {
public:
    RedisCommandHandler();

    //process command from client and return RESP-formatted response.
    std::string processCommand(const std::string& commandLine);
private:
};

#endif