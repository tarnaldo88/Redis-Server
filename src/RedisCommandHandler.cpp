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

// static std::string handlePing(const std::vector<std::string>& tokens, RedisDatabase& /*db*/) {
//         return "+PONG\r\n";
//     }

// static std::string handleEcho(const std::vector<std::string>& tokens, RedisDatabase& /*db*/) {
//     if (tokens.size() < 2)
//         return "-Error: ECHO requires a message\r\n";
//     return "+" + tokens[1] + "\r\n";
// }

// static std::string handleFlushAll(const std::vector<std::string>& /*tokens*/, RedisDatabase& db) {
//     // db.flushAll();
//     return "+OK\r\n";
// }

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
    if (cmd == "PING") {
        // RESP: simple string +PONG or echo back message if provided
        if (tokens.size() >= 2) {
            response << "+" << tokens[1] << "\r\n";
        } else {
            response << "+PONG\r\n";
        }
    } else if (cmd == "ECHO") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'echo' command\r\n";
        } else {
            const std::string& msg = tokens[1];
            response << "$" << msg.size() << "\r\n" << msg << "\r\n";            
        }
    } else if (cmd == "FLUSHALL") {
        // If a real flush exists, call it on db; otherwise acknowledge
        db.flushAll();
        response << "+OK\r\n";
    }
    // Key/Value operations
    else if(cmd == "SET")
    {
        if(tokens.size() < 3){
            response << "-ERR wrong number of arguments for 'SET' command. Requires Key and Value\r\n";
        }

        db.set(tokens[1], tokens[2]);
        response << "+OK\r\n";
    }
    else if(cmd == "GET")
    {
        if(tokens.size() < 3){
            response << "-ERR wrong number of arguments for 'GET' command. Requires Key and Value\r\n";
        } else {
            std::string val;
            if(db.get(tokens[1], tokens[2])){
                response << "$" << val.size() << "\r\n" << val << "\r\n";
            } else {
                response << "$-1\r\n";
            }            
        }        
    } 
    else if(cmd == "KEYS")
    {
        std::vector<std::string> allKeys = db.keys();
        response << "*" << allKeys.size() << "\r\n";

        for(const auto& key : allKeys){
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    else if(cmd == "TYPE")
    {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'TYPE' command. Requires key\r\n";
        } else {            
            response << "+" << db.type(tokens[1]) << "\r\n";
        }
    }
    else if(cmd == "DEL" || cmd == "UNLINK")
    {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for " << cmd << "command. Requires key\r\n";
        } else {
            bool res = db.del(tokens[1]);
            response << ":" << res << "\r\n";
        }
    }
    else if (cmd =="EXPIRE")
    {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for " << cmd << "command. Requires key and seconds.\r\n";
        } else {
            if(db.expire(tokens[1], std::stoi(tokens[2]))){
                response << "+OK\r\n";
            } else {

            }            
        }
    }
    else if(cmd == "RENAME")
    {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for " << cmd << "command. Requires old key and new key.\r\n";
        } else {
            if(db.rename(tokens[1], tokens[2])) {
                response << "+OK\r\n";
            } else {
                
            }

        }
    }
    // List Operations
    // Hash Operations
    else {
        response << "-ERR unknown command '" << tokens[0] << "'\r\n";
    }

    return response.str();
}