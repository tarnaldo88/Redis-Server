#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

class RedisServer{
public:
    RedisServer(int port);
private:
    int port;
};

#endif