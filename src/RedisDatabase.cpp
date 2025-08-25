#include "RedisCommandHandler.h"
#include "RedisDatabase.h"

RedisDatabase &RedisDatabase::getInstance()
{    
    static RedisDatabase instance;
    return instance;
}

// Key/Value operations
// List Operations
// Hash Operations

/*
Memory -> file - dump()
file -> load() memory when opening 

K = key value
L = list
H = hash
*/

bool RedisDatabase::dump(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);

    if(!ofs) return false;

    for(const auto& kv : kv_store){
        ofs << "K" << kv.first << " " << kv.second;
    }

    for(const auto& kv : list_store){
        ofs << "L" << kv.first;
        for(const auto& item : kv.second){
            ofs << " " << item;
        }
        ofs << "\n";
    }

    for(const auto& kv : hash_store){
        ofs << "L" << kv.first;
        for(const auto& item : kv.second){
            ofs << item.first << ":" << item.second;            
        }
        ofs << "\n";
    }

    return true;
}

bool RedisDatabase::load(const std::string &filename)
{
    return false;
}
