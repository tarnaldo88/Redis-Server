#include "main.h"
#include "RedisServer.h"
#include "RedisDatabase.h"

int main(int argc, char* argv[]) {
    //default port for now
    int port = 6379;
    if(argc >=2) port = std::stoi(argv[1]);

    if(RedisDatabase::getInstance().load("dump.my_rdb")) {
        std::cout << "Database loaded from dump.my_rdb.\n";
    } else {
        std::cout << "No dump found or load failed. Starting with empty database.\n";
    }

    RedisServer server(port);

    //Background persistannce: Every 5 minutes save database
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            if(!RedisDatabase::getInstance().dump("dump.my_rdb")) {
                std::cerr << "Error dumping Database. \n";
            } else {
                std::cout <<"Database dumped to dump.my_rdb\n";
            }            

        }
    });

    persistanceThread.detach();

    server.run();

    return 0;
}