#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>

class RedisCommandHandler {
public:
    RedisCommandHandler();

    //process command from client and return RESP-formatted response.
    std::string processCommand(const std::string& commandLine);
private:
};

#endif