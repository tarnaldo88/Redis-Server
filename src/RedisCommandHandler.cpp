#include "./include/RedisCommandHandler.h"

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

    for(int i = 0; i< numElements; i++ ){
        if ( pos>=input.size() || input[pos] != '$'){
            break; //format error
        }
    }

}