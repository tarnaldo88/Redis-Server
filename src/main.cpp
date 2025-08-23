#include "./include/main.h"
#include "./include/RedisServer.h"

int main(int argc, char* argv[]) {
    //default port for now
    int port = 6379;
    if(argc >=2) port = std::stoi(argv[1]);

    RedisServer server(port);

    //Background persistannce: Every 5 minutes save database
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            // if(!RedisDatabase) {}
            //dump database

        }
    });

    persistanceThread.detach();

    server.run();

    return 0;
}