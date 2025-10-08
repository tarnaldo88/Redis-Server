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
    if (!ofs) return false;

    for (const auto& kv: kv_store) {
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }
    for (const auto& kv : list_store) {
        ofs << "L " << kv.first;
        for (const auto& item : kv.second)
            ofs << " " << item;
        ofs << "\n";
    }
    for (const auto& kv : hash_store) {
        ofs << "H " << kv.first;
        for (const auto& field_val : kv.second) 
            ofs << " " << field_val.first << ":" << field_val.second;
        ofs << "\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K') {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        } else if (type == 'L') {
            std::string key;
            iss >> key;
            std::string item;
            std::vector<std::string> list;
            while (iss >> item)
                list.push_back(item);
            list_store[key] = list;
        } else if (type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;
            while (iss >> pair) {
                auto pos = pair.find(':');
                if (pos != std::string::npos) {
                    std::string field = pair.substr(0, pos);
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

std::string RedisDatabase::getSet(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    std::string oldValue = kv_store[key];
    kv_store[key] = value;
    return oldValue;
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
    purgeExpired();
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
    purgeExpired();

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
    purgeExpired();
    bool erased = false;
    erased |= kv_store.erase(key) > 0;
    erased |= list_store.erase(key) > 0;
    erased |= hash_store.erase(key) > 0;
    
    return erased;
}

bool RedisDatabase::expire(const std::string &key, int seconds)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    bool expired = (kv_store.find(key) != kv_store.end()) || 
                    (list_store.find(key) != list_store.end() || 
                    (hash_store.find(key) != hash_store.end()));

    if(!expired) return false;

    expire_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}

void RedisDatabase::purgeExpired()
{
    // std::lock_guard<std::mutex> lock(db_mutex);
    auto now = std::chrono::steady_clock::now();
    
    for(auto it = expire_map.begin(); it != expire_map.end(); ) {
        if(now > it->second) {
            //Remove from all the stores if expired
            kv_store.erase(it->first);
            list_store.erase(it->first);
            hash_store.erase(it->first);
            it = expire_map.erase(it);
        } else {
            ++it;
        }
    }
}

bool RedisDatabase::rename(const std::string &oldKey, const std::string &newKey)
{    
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    bool found = false;

    auto itKv = kv_store.find(oldKey);

    if(itKv != kv_store.end()){
        kv_store[newKey] = itKv->second;
        kv_store.erase(oldKey);
        found = true;
    }

    auto itList = list_store.find(oldKey);

    if(itList != list_store.end()){
        list_store[newKey] = itList->second;
        list_store.erase(oldKey);
        found = true;
    }

    auto itMap = hash_store.find(oldKey);

    if(itMap != hash_store.end()){
        hash_store[newKey] = itMap->second;
        hash_store.erase(oldKey);
        found = true;
    }

    auto itExpire = expire_map.find(oldKey);

    if(itExpire != expire_map.end()){
        expire_map[newKey] = itExpire->second;
        expire_map.erase(oldKey);
        found = true;
    }

    return found;                                                                                                                       
}

//LIST 

ssize_t RedisDatabase::llen(const std::string& key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);
    if(it != list_store.end()){
        return it->second.size();
    } else {
        return 0;
    }
}

std::vector<std::string> RedisDatabase::Lget(const std::string& key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second; 
    }
    return {}; 
}

bool RedisDatabase::lindex(const std::string& key, int index, std::string& value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);

    if(it == list_store.end()) return false;
    
    const auto& lst = it->second;
    // int vecSize = static_cast<int>(lst.size());

    if(index < 0) {
        index = lst.size() + index;
    }
   
    //if still less than zero after giving list size, return false
    if(index < 0 || index >= static_cast<int>(lst.size())){
        return false;
    }

    value = lst[index];
    return true;
}

bool RedisDatabase::lSet(const std::string &key, int index, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);

    if(it == list_store.end()) return false;
    
    auto& lst = it->second;

    if(index < 0) {
        index = lst.size() + index;
    }
   
    //if still less than zero after giving list size, return false
    if(index < 0 || static_cast<size_t>(index) >= lst.size()){
        return false;
    }
    
    lst[index] = value;
    return true;
}

int RedisDatabase::lRemove(const std::string &key,  int count, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);
    if (it == list_store.end()) return 0;  // no such list

    auto& vec = it->second;
    int removed = 0;
    // Remove all elements equal to element.
    if(count == 0)
    {
        auto new_end = std::remove(vec.begin(), vec.end(), value);
        removed = std::distance(new_end, vec.end());
        vec.erase(new_end, vec.end());        
    }
    //Remove elements equal to element moving from head to tail.
    else if( count > 0)
    {
        for (auto iter = vec.begin(); iter != vec.end() && removed < count;) {
            if (*iter == value) {
                iter = vec.erase(iter);
                ++removed;
            } else {
                ++iter;
            }
        }
    } 
    // Remove elements equal to element moving from tail to head.
    else
    {
        for (auto riter = vec.rbegin(); riter != vec.rend() && removed < -count;) {
            if(*riter == value){
                auto fwdIter = riter.base();
            --fwdIter;
            fwdIter = vec.erase(fwdIter);
            ++removed;
            riter = std::reverse_iterator(fwdIter);
            } else {
                ++riter;
            } 
        }
    }
    
    return removed;
}

void RedisDatabase::lpush(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    list_store[key].insert(list_store[key].begin(), value);
}

void RedisDatabase::lpush(const std::string &key, const std::vector<std::string> &values)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();

    //loop through adding in values
    for(const auto& value : values){
        list_store[key].insert(list_store[key].begin(), value);    
    }    
}

void RedisDatabase::rpush(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    list_store[key].emplace_back(value);
}

void RedisDatabase::rpush(const std::string &key, const std::vector<std::string> &values)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    //loop and add in values
    for(const auto& value : values){
        list_store[key].emplace_back(value);
    }    
}

bool RedisDatabase::lpop(const std::string &key, std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);

    if(it != list_store.end() && !it->second.empty())
    {
        value = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    } 
    else 
    {
        return false;
    }    
}

bool RedisDatabase::rpop(const std::string &key, std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = list_store.find(key);

    if(it != list_store.end() && !it->second.empty())
    {
        value = it->second.back();
        it->second.pop_back();
        return true;
    } 
    else 
    {
        return false;
    }    
}

// HASH OPERATIONS

ssize_t RedisDatabase::Hlen(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = hash_store.find(key);
    if(it != hash_store.end()){
        return it->second.size();
    } else {
        return 0;
    }
}

bool RedisDatabase::Hset(const std::string &key, const std::string &field, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    hash_store[key][field] = value;
    return true;
}

bool RedisDatabase::Hget(const std::string &key, const std::string &field, std::string& value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = hash_store.find(key);
    if(it != hash_store.end()){
        auto secondIt = hash_store[key].find(field);
        if(secondIt != it->second.end()){
            value = secondIt->second;
            return true;
        }        
    } 
    return false;
}

bool RedisDatabase::Hexists(const std::string &key, const std::string &field)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = hash_store.find(key);
    if(it != hash_store.end()){
        return (it->second.find(field) != hash_store[key].end());
    } else {
        return false;
    }
}

bool RedisDatabase::Hdel(const std::string &key, const std::string &field)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = hash_store.find(key);
    if(it != hash_store.end()){
        return it->second.erase(field) > 0;
    } else {
        return false;
    }

}

std::vector<std::string> RedisDatabase::Hkeys(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    std::vector<std::string> keysVec;
    auto it = hash_store.find(key);

    if(it != hash_store.end())
    {
        for(const auto& pair : it->second)
        {
            keysVec.emplace_back(pair.first);
        }
        return keysVec;
    } 

    return keysVec;    
}

std::vector<std::string> RedisDatabase::Hvals(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    std::vector<std::string> valuesVec;
    auto it = hash_store.find(key);

    if(it != hash_store.end())
    {
        for(const auto& pair : it->second)
        {
            valuesVec.emplace_back(pair.second);
        }
    }

    return valuesVec;
}

std::unordered_map<std::string, std::string> RedisDatabase::Hgetall(const std::string &key)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    std::unordered_map<std::string, std::string> all;    

    if(hash_store.find(key) != hash_store.end())
    {
        return hash_store[key];
    }
    return {};
}

bool RedisDatabase::HMset(const std::string &key, const std::vector<std::pair<std::string, std::string>> &fieldValues)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    for(const auto& pair : fieldValues)
    {
        hash_store[key][pair.first] = pair.second;
    }
    return true;
}

bool RedisDatabase::Hsetnx(const std::string &key, const std::string &field, const std::string &value)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    auto it = hash_store.find(key);
    if(it != hash_store.end())
    {
        hash_store[key][field] = value;
        return true;
    }
    return false;
}

bool RedisDatabase::Hrandfield(const std::string &key, std::vector<std::string> &value, const int &count)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();

    emplace_rand_fields(key, value, count);
    
    return true;
}

void emplace_rand_fields(const std::string &key, std::vector<std::string> &value, const int &count)
{
    // Create a random device to seed the generator
    std::random_device rd;

    // Create a Mersenne Twister generator (fast and high-quality)
    std::mt19937 gen(rd());

    // Define the range [min, max]
    int min = 0;
    int max = hash_store[key].size() - 1;

    // Create a uniform distribution
    std::uniform_int_distribution<> dist(min, max);

    for (int i = 0; i < count; ++i) {
        int random_index = dist(gen);

        // advance iterator to random index
        auto it = mymap.begin();
        std::advance(it, random_index);

        value.emplace_back(it->second);
    }   
}

bool RedisDatabase::Hscan(const std::string &key, const int &cursor, std::vector<std::string> &values)
{
    std::lock_guard<std::mutex> lock(db_mutex);
    purgeExpired();
    
    return true;
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

EXAMPLE DUMP FILE CONTENTS

K name Alice
K city moorpark
L stuff things cosas potatoes
L prgl c++ python java c javascript sql
H user:100 name:arnaldo age:90 email:test@test.com
H user:230 name:MrTest age:120 email:tesasdfasft@test.com

K city moorpark
K name Arnie
K test testers
K session:1 data
L mm t a c a d a b c
L stuff things cosas potatoes
L prgl c++ python java c javascript sql
L mylist a r t e q ee rr tt
L milist b c s sdd dsd ds dd ww
H user:100 gg : testes f : t c : v a : b
H user:230 mm : testes f : t c : v a : b
H mm test : testes f : t c : v a : b

*/