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
    static std::string handlePing(const std::vector<std::string>& /*tokens*/, RedisDatabase& /*db*/);
    static std::string handleEcho(const std::vector<std::string>& tokens, RedisDatabase& /*db*/);
    static std::string handleFlushAll(const std::vector<std::string>& /*tokens*/, RedisDatabase& db);
};

#endif