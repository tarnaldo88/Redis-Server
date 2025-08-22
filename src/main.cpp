#include "./include/main.h"
#include "./include/RedisServer.h"

int main(int argc, char* argv[]) {
    //default port for now
    int port = 6379;
    if(argc >=2) port = std::stoi(argv[1]);

    return 0;
}