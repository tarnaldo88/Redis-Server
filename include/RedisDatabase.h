#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <random>

class RedisDatabase {
public:
    //Get the singleton instance
    static RedisDatabase& getInstance();

    //Persistance: Dump /Load the database from a file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

    //Common Commands
    bool flushAll();

    //Key Value commands
    void set(const std::string& key, const std::string& value);
    std::string getSet(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string& key);
    bool del(const std::string& key);
    //expire
    bool expire(const std::string& key, int seconds);
    //rename
    bool rename(const std::string& oldKey, const std::string& newKey);

    //List Operations
    ssize_t llen(const std::string& key);
    std::vector<std::string> Lget(const std::string& key);
    bool lindex(const std::string& key, int index, std::string& value);
    bool lSet(const std::string& key, int index, const std::string& value);
    int lRemove(const std::string& key, int count, const std::string& value);
    void lpush(const std::string& key, const std::string& value);
    void lpush(const std::string& key, const std::vector<std::string>& values);
    void rpush(const std::string& key, const std::string& value);
    void rpush(const std::string& key, const std::vector<std::string>& values);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string& key, std::string& value);
    int linsert(const std::string& key, const std::string& value, const std::string& pivot);
    

    //Hash Operations
    ssize_t Hlen(const std::string& key);
    bool Hset(const std::string& key, const std::string& field, const std::string& value);
    bool Hget(const std::string& key, const std::string &field, std::string& value);
    bool Hexists(const std::string& key, const std::string& field);
    bool Hdel(const std::string& key, const std::string& field);
    std::vector<std::string> Hkeys(const std::string& key);
    std::vector<std::string> Hvals(const std::string& key);
    std::unordered_map<std::string, std::string> Hgetall(const std::string& key);
    bool HMset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& fieldValues);
    bool Hsetnx(const std::string& key, const std::string& field, const std::string& value);
    bool Hrandfield(const std::string& key, std::vector<std::string>& value, const int& count);
    bool Hscan(const std::string& key, const int& cursor, std::vector<std::string>& values);

private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

    void purgeExpired();
    void emplace_rand_fields(const std::string &key, std::vector<std::string> &value, const int &count);

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_map;

};
#endif
