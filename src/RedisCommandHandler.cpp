#include "RedisCommandHandler.h"
#include "RedisDatabase.h"


//RESP parser:
//2*\r\n$4\r\n\PING\r\n$4\r\n$4\r\nTEST\r\n
//2* -> array has 2  elements
//$4 -> next string has 4 chars
//PING 
//TEST

std::vector<std::string> parseRespCommand(const std::string &input){
    std::vector<std::string> tokens;

    if(input.empty()){
        return tokens;
    }

    //if does not start with * , then start splitting whitespaces
    if(input[0] != '*') {
        std::istringstream iss(input);
        std::string token;

        while(iss >> token) {
            tokens.emplace_back(token);
        }

        return tokens;
    }

    size_t pos = 0;

    //expect * followed by number of elements
    if(input[pos] != '*'){
        return tokens;
    }
    
    pos++;

    //carriage return line feed
    size_t crlf = input.find("\r\n", pos);

    if(crlf == std::string::npos) return tokens;

    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for(int i = 0; i < numElements; i++ ){
        if ( pos>=input.size() || input[pos] != '$'){
            break; //format error
        }
        pos++; //skip $

        crlf = input.find("\r\n", pos);

        if(crlf == std::string::npos) break;

        int len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        if(pos + len > input.size()){
            break; //out of inputs
        }

        std::string token = input.substr(pos,len);
        tokens.emplace_back(token);
        pos += len + 2; //skip token & crlf
    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler() {}

static std::string handlePing(const std::vector<std::string>& /*tokens */, RedisDatabase& /*db*/) {
        return "+PONG\r\n";
    }

static std::string handleEcho(const std::vector<std::string>& tokens, RedisDatabase& /*db*/) {
    if (tokens.size() < 2) {
        return "-Error: ECHO requires a message\r\n";
    }        
    const std::string& msg = tokens[1];
    return "$" + std::to_string(msg.size()) + "\r\n" + msg + "\r\n"; 
}

static std::string handleFlushAll(const std::vector<std::string>& /*tokens*/, RedisDatabase& db) {
    db.flushAll();
    return "+OK\r\n";
}

static std::string handleSet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size() < 3)
        return "-Error: SET requires key and value\r\n";
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleGet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size() < 2){
        return "-ERR wrong number of arguments for 'GET' command.\r\n";
    } else {
        std::string val;
        if(db.get(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        } else {
            return "$-1\r\n";
        }            
    }  
}

static std::string handleType(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-ERR wrong number of arguments for 'TYPE' command. Requires key\r\n";
    } else {            
        return + "+" + db.type(tokens[1]) + "\r\n";
    }
}

static std::string handleDel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-ERR wrong number of arguments for " + tokens[0] + "command. Requires key\r\n";
    } else {
        bool res = db.del(tokens[1]);
        return ":" + std::string(res ? "1" : "0") + "\r\n";
    }
}

static std::string handleKeys(const std::vector<std::string>& tokens, RedisDatabase& db) {
    std::vector<std::string> allKeys = db.keys();
    std::ostringstream response;

    response << "*" << allKeys.size() << "\r\n";

    for(const auto& key : allKeys){
        response << "$" << key.size() << "\r\n" << key << "\r\n";
    }
    return response.str();
}

static std::string handleExpire(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-ERR wrong number of arguments for " + tokens[0] + "command. Requires key and seconds.\r\n";
    } else {
        int seconds = std::stoi(tokens[2]);        
        if(db.expire(tokens[1], seconds)){
            return + "+OK\r\n";
        } else {
            return "-Error: Key not found\r\n";
        }            
    }
}

static std::string handleRename(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: RENAME requires old key and new key\r\n";
    }
     
    if (db.rename(tokens[1], tokens[2])) {
        return "+OK\r\n";
    }        
    return "-Error: Key not found or rename failed\r\n";
}

static std::string handleLlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size() < 2){
        return "-ERR LLEN requires a key.\r\n";
    } else {
        ssize_t len = db.llen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";
    }
}

static std::string handleLGet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    std::vector<std::string> allElements = db.Lget(tokens[1]);
    std::ostringstream response;

    response << "*" << allElements.size() << "\r\n";

    for(const auto& element : allElements){
        response << "$" << element.size() << "\r\n" << element << "\r\n";
    }
    return response.str();
}

static std::string handleLindex(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-ERR LINDEX requires key and index.\r\n";
    } else { 
        try{
            int index = std::stoi(tokens[2]);
            std::string value;
            if (db.lindex(tokens[1], index, value)){
                return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
            } else {
                return "$-1\r\n";
            }
        } catch(const std::exception&) {
            return "-Error: invalid index\r\n";
        }
    }
}

static std::string handleLset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) {
        return "-ERR LSET requires key, index, and value.\r\n";
    } else {            
        try {
            int index = std::stoi(tokens[2]);
            if(db.lSet(tokens[1], index, tokens[3])){
                return "+OK\r\n";
            } else {
                return "-Error: Index out of range\r\n";
            }
        } catch (const std::exception&){
            return "-Error: invalid index\r\n";
        }
    }
}

static std::string handleLrem(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) {
        return "-ERR LREM requires key, count, and value.\r\n";
    } else {            
        int count = std::stoi(tokens[2]);
        int removed = db.lRemove(tokens[1], count, tokens[3]);
        return ":" + std::to_string(removed) + "\r\n";
    }
}
        
static std::string handleLPush(const std::vector<std::string>& tokens, RedisDatabase& db){
    if(tokens.size() < 2){
        return "-ERR LPUSH requires key and value.\r\n";
    } else {
        std::vector<std::string> values;
        
        //loop through tokens to grab all the arguments input after the key.
        for(size_t i = 2; i < tokens.size(); i++){
            values.emplace_back(tokens[i]);
        }
        db.lpush(tokens[1], values);
        ssize_t len = db.llen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";
    }
    
}

static std::string handleLPop(const std::vector<std::string>& tokens, RedisDatabase& db){
    if(tokens.size() < 2){
        return "-ERR LPOP requires key.\r\n";
    } else {
        std::string val;
        if(db.lpop(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        } else {
            return "$-1\r\n";
        }        
    }    
}

static std::string handleRPush(const std::vector<std::string>& tokens, RedisDatabase& db){
    if(tokens.size() < 2){
        return "-ERR LPUSH requires key and value.\r\n";
    } else {
        std::vector<std::string> values;

        //loop through tokens to grab all the arguments input after the key.
        for(size_t i = 2; i < tokens.size(); i++){
            values.emplace_back(tokens[i]);
        }
        db.rpush(tokens[1], values);
        ssize_t len = db.llen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";
    }
}

static std::string handleRPop(const std::vector<std::string>& tokens, RedisDatabase& db){
    if(tokens.size() < 2){
        return "-ERR RPOP requires key\r\n";
    } else {
        std::string val;
        if(db.rpop(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        } else {
            return "$-1\r\n";
        }        
    }
}

///HASH HANDLE FUNCTIONS
static std::string handleHset(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 4){
        return "-Error: HSET requires key, field, and value.\r\n";
    } else {
        bool result = db.Hset(tokens[1], tokens[2], tokens[3]);
        return ":"+ std::to_string(result ? 1 : 0) + "\r\n";
    }
}

static std::string handleHget(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 3){
        return "-Error: HGET requires key and field.\r\n";
    } else {
        std::string value;
        if(db.Hget(tokens[1], tokens[2], value)){
            return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
        } else {
          return "$-1\r\n"  ;
        }
    }
}

static std::string handleHexists(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 3){
        return "-Error: HEXISTS requires key and field.\r\n";
    } else {        
        bool exists = db.Hexists(tokens[1], tokens[2]);
        return ":" + std::to_string(exists ? 1 : 0) + "\r\n";
    }
}

static std::string handleHdel(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 3){
        return "-Error: HDEL requires key and field.\r\n";
    } else {
        bool result = db.Hdel(tokens[1], tokens[2]);
        return ":" + std::to_string(result ? 1 : 0) + "\r\n";
    }
}

static std::string handleHlen(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 2){
        return "-Error: HLEN requires key.\r\n";
    } else {
        ssize_t len = db.Hlen(tokens[1]);
        return ":" + std::to_string(len) + "\r\n";
    }
}

static std::string handleHvals(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 2){
        return "-Error: HVALS requires key.\r\n";
    } else {
        std::vector<std::string> allValues = db.Hvals(tokens[1]);
        std::ostringstream response;

        response << "*" << allValues.size() << "\r\n";

        for(const auto& value : allValues){
            response << "$" << value.size() << "\r\n" << value << "\r\n";
        }
        return response.str();
    }
}

static std::string handleHgetall(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 2){
        return "-Error: HGETALL requires key.\r\n";
    } else {
        auto hash = db.Hgetall(tokens[1]);
        std::ostringstream oss;
        oss << "*" << hash.size() * 2 << "\r\n";
        for(const auto& pair : hash){
            oss << "$" << std::to_string(pair.first.size()) << "\r\n" << pair.first << "\r\n";
            oss << "$" << std::to_string(pair.second.size()) << "\r\n" << pair.second << "\r\n";
        }
        return oss.str();
    }
}

static std::string handleHkeys(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 2){
        return "-Error: HKEYS requires key.\r\n";
    } else {
        std::vector<std::string> allKeys = db.Hkeys(tokens[1]);
        std::ostringstream response;

        response << "*" << allKeys.size() << "\r\n";

        for(const auto& key : allKeys){
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
        return response.str();
    }    
}

static std::string handleHmset(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 4 || (tokens.size() % 2 == 1)){
        return "-Error: HMSET requires key followed by field value pairs.\r\n";
    } else {
        std::vector<std::pair<std::string, std::string>> fieldVals;

        //iterate through the input to populate fieldVals before sending to HMset function in RedisDatabase
        for(size_t i = 2; i < tokens.size(); i += 2){
            fieldVals.emplace_back(std::pair(tokens[i],tokens[i+1]));            
        }

        bool result = db.HMset(tokens[1], fieldVals);
        return ":" + std::to_string(result ? 1 : 0) + "\r\n";
    }
}

//Additional Commands
static std::string handleGetSet(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 3)
        return "-Error: GETSET requires key and value\r\n";
    std::string oldValue = db.getSet(tokens[1], tokens[2]);
    return oldValue;
}

static std::string handleHsetnx(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    if(tokens.size() < 4)
        return "-Error: HSETNX requires key, field, and value\r\n";
    bool result = db.Hsetnx(tokens[1], tokens[2], tokens[3]);
    return ":" + std::to_string(result ? 1 : 0) + "\r\n";
}

static std::string handleHrandfield(const std::vector<std::string>& tokens, RedisDatabase& db)
{
    //check if count is provided
    if(tokens.size() < 3)
        return "-Error: HRANDFIELD requires key and count\r\n";
    
    std::vector<std::string> values;
    db.Hrandfield(tokens[1], values, std::stoi(tokens[2]));
    
    std::ostringstream response;

        response << "*" << values.size() << "\r\n";
        int i = 1;

        for(const auto& key : values){
            response << "$" << key.size() << "\r\n" << i << ")" <<key << "\r\n";
            i++;
        }
        return response.str();
}

std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    //Using RESP parser;
    std::vector<std::string> tokens = parseRespCommand(commandLine);

    if(tokens.empty()) {
        return ""; // ignore empty commands
    } 

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    RedisDatabase& db = RedisDatabase::getInstance();

    //check commands
    //First check for Common Commands
    if (cmd == "PING") 
    {
        // RESP: simple string +PONG or echo back message if provided        
        return handlePing(tokens, db);        
    } 
    else if (cmd == "ECHO") 
    {
        return handleEcho(tokens,db) ;        
    } 
    else if (cmd == "FLUSHALL") 
    {
        // If a real flush exists, call it on db; otherwise acknowledge
        return handleFlushAll(tokens,db);
    }
    // Key/Value Operations
    else if(cmd == "SET")
    {
        return handleSet(tokens, db);
    }
    else if(cmd == "GET")
    {
        return handleGet(tokens, db);
    } 
    else if(cmd == "KEYS")
    {
        return handleKeys(tokens, db);
    }
    else if(cmd == "TYPE")
    {
        return handleType(tokens, db);
    }
    else if(cmd == "DEL" || cmd == "UNLINK")
    {
        return handleDel(tokens,db);
    }
    else if (cmd =="EXPIRE")
    {
        return handleExpire(tokens, db);
    }
    else if(cmd == "RENAME")
    {
        return handleRename(tokens, db);
    }
    else if(cmd == "GETSET")
    {
        return handleGetSet(tokens, db);
    }
    // List Operations
    else if(cmd == "LLEN")
    {
        return handleLlen(tokens, db);
    }
    else if(cmd == "LGET")
    {
        return handleLGet(tokens, db);
    }
    else if(cmd == "LINDEX")
    {
        return handleLindex(tokens, db);
    }
    else if(cmd == "LSET")
    {
        return handleLset(tokens, db);
    }
    else if(cmd == "LREM")
    {
        return handleLrem(tokens, db);
    }
    else if(cmd == "LPUSH")
    {
        return  handleLPush(tokens,db);
    }
    else if(cmd == "RPUSH")
    {
        return  handleRPush(tokens,db);
    }
    else if(cmd == "LPOP")
    {
        return  handleLPop(tokens,db);
    }
    else if( cmd == "RPOP")
    {
        return  handleRPop(tokens,db);
    }
    // Hash Operations
    else if( cmd == "HSET") 
    {
        return handleHset(tokens, db);
    }
    else if( cmd == "HDEL") 
    {
        return handleHdel(tokens, db);
    }
    else if( cmd == "HGET") 
    {
        return handleHget(tokens, db);
    }
    else if( cmd == "HEXISTS") 
    {
        return handleHexists(tokens, db);
    }
    else if( cmd == "HLEN") 
    {
        return handleHlen(tokens, db);
    }
    else if( cmd == "HVALS") 
    {
        return handleHvals(tokens, db);
    }
    else if( cmd == "HGETALL") 
    {
        return handleHgetall(tokens, db);
    }
    else if( cmd == "HMSET") 
    {
        return handleHmset(tokens, db);
    } 
    else if( cmd == "HKEYS") 
    {
        return handleHkeys(tokens, db);
    }
    else if(cmd == "HSETNX")    
    {
        return handleHsetnx(tokens, db);
    }
    else if(cmd == "HRANDFIELD")
    {
        return handleHrandfield(tokens, db);
    }
    else
    {
        return "-ERR unknown command '" + tokens[0] + "'\r\n";
    }
}

/*

Hashes
HSET: HSET <key> <field> <value>
HGET: HGET <key> <field>
HEXISTS: HEXISTS <key> <field>
HDEL: HDEL <key> <field>
HLEN: HLEN <key> → field count
HKEYS: HKEYS <key> → all fields
HVALS: HVALS <key> → all values
HGETALL: HGETALL <key> → field/value pairs
HMSET: HMSET <key> <f1> <v1> [f2 v2 ...]

*/

