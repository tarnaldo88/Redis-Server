#include "RedisCommandHandler.h"
#include "RedisDatabase.h"

RedisDatabase &RedisDatabase::getInstance()
{    
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::dump(const std::string &filename)
{
    return false;
}

bool RedisDatabase::load(const std::string &filename)
{
    return false;
}
