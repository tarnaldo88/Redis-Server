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
            tokens.push_back(token);
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
        tokens.push_back(token);
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
    if(tokens.size() < 2){
        return "-ERR wrong number of arguments for 'GET' command. Requires Key and Value\r\n";
    } else {
        std::string val;
        if(db.get(tokens[1], val)){
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        } else {
            return "$-1\r\n";
        }            
    }  
}

static std::string handleGet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size() < 2){
        return "-ERR wrong number of arguments for 'GET' command. Requires Key and Value\r\n";
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
        return ":" + std::string(res ? "true" : "false") + "\r\n";
    }
}

static std::string handleKeys(const std::vector<std::string>& tokens, RedisDatabase& db, std::ostringstream& response) {
    std::vector<std::string> allKeys = db.keys();
    response << "*" << allKeys.size() << "\r\n";

    for(const auto& key : allKeys){
        response << "$" << key.size() << "\r\n" << key << "\r\n";
    }
    return response.str();
}

//TODO FINISH THIS FUNC
static std::string handleExpire(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-ERR wrong number of arguments for " + tokens[0] + "command. Requires key and seconds.\r\n";
    } else {
        if(db.expire(tokens[1], std::stoi(tokens[2]))){
            return + "+OK\r\n";
        } else {

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
        

std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    //Using RESP parser;
    std::vector<std::string> tokens = parseRespCommand(commandLine);

    if(tokens.empty()) {
        return ""; // ignore empty commands
    } 
    
    //for debugging
    // std::cout << commandLine<< "\n";    
    // for(auto& t : tokens){
    //     std::cout << t << "\n";
    // }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
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
    // Key/Value operations
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
        return handleKeys(tokens, db, response);

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
    // List Operations
    // Hash Operations
    else {
        return "-ERR unknown command '" + tokens[0] + "'\r\n";
    }
}