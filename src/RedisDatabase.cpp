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
        ofs << "H" << kv.first;
        for(const auto& item : kv.second){
            ofs << item.first << ":" << item.second;            
        }
        ofs << "\n";
    }

    return true;
}

bool RedisDatabase::load(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);

    if(!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;

    while(std::getline(ifs, line)){
        std::istringstream iss(line);
        char type;

        iss >> type;

        if(type == 'K')
        {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        } 
        else if(type == 'L')
        {
            std::string key;
            iss >> key;
            std::string item;
            std::vector<std::string> list;

            while(iss >> item){
                list.push_back(item);
            }
            list_store[key] = list;
        } 
        else if(type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string>hash;
            std::string pair;
            
            while(iss >> pair){
                auto pos = pair.find(":");
                if(pos != std::string::npos){
                    std::string field = pair.substr(0,pos);
                    std::string value = pair.substr(pos+1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }

    }

    return true;
}

bool RedisDatabase::flushAll()
{
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

void RedisDatabase::set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;    
}

bool RedisDatabase::get(const std::string &key, std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if(it != kv_store.end()) {
        value = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> RedisDatabase::keys()
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> result;

    for(const auto& pair :kv_store){
        result.push_back(pair.first);
    }

    for(const auto& pair :list_store){
        result.push_back(pair.first);
    }

    for(const auto& pair :hash_store){
        result.push_back(pair.first);
    }
    return result;
}

std::string RedisDatabase::type(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    if(kv_store.find(key) != kv_store.end()) {
        return "string";
    }

    if(list_store.find(key) != list_store.end()) {
        return "list";
    }

    if(hash_store.find(key) != hash_store.end()) {
        return "hash";
    } else {
        return "none";
    }    
}

bool RedisDatabase::del(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    erased |= kv_store.erase(key) > 0;
    erased |= list_store.erase(key) > 0;
    erased |= hash_store.erase(key) > 0;
    
    return erased;
}

bool RedisDatabase::expire(const std::string &key, int seconds)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    bool expired = (kv_store.find(key) != kv_store.end()) || 
                    (list_store.find(key) != list_store.end() || 
                    (hash_store.find(key) != hash_store.end()));

    if(!expired) return false;

    expire_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}

bool RedisDatabase::rename(const std::string &oldKey, const std::string &newKey)
{    
    std::lock_guard<std::mutex> lock(db_mutex);
    bool found = false;

    auto itKv = kv_store.find(oldKey);

    if(itKv != kv_store.end()){
        kv_store[newKey] = itKv->second;
        kv_store.erase(oldKey);
        found = true;
    }

    auto itLv = list_store.find(oldKey);

    if(itLv != list_store.end()){
        list_store[newKey] = itLv->second;
        list_store.erase(oldKey);
        found = true;
    }

    auto itMv = hash_store.find(oldKey);

    if(itMv != hash_store.end()){
        hash_store[newKey] = itMv->second;
        hash_store.erase(oldKey);
        found = true;
    }

    auto itEv = expire_map.find(oldKey);

    if(itEv != expire_map.end()){
        expire_map[newKey] = itEv->second;
        expire_map.erase(oldKey);
        found = true;
    }

    return found;                                                                                                                       
}

/*
Key-Value (K)
kv_store["name"] = "Alice";
kv_store["city"] = "Berlin";

List (L)
list_store["fruits"] = {"apple", "banana", "orange"};
list_store["colors"] = {"red", "green", "blue"};

Hash (H)
hash_store["user:100"] = {
    {"name", "Bob"},
    {"age", "30"},
    {"email", "bob@example.com"}
};

hash_store["user:200"] = {
    {"name", "Eve"},
    {"age", "25"},
    {"email", "eve@example.com"}
};
*/