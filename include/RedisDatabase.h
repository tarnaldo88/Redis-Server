#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

class RedisDatabase {
public:
    //Get the singleton instance
    static RedisDatabase& getInstance();


private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

};

#endif