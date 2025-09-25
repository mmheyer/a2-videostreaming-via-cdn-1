#include <iostream>
#include "DNSServer.h"
#include "../common/Logger.hpp"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: ./nameserver [--geo|--rr] <port> <servers> <log>" << std::endl;
        return 1;
    }
    std::string mode = argv[1];
    int port = atoi(argv[2]);
    std::string serverFile = argv[3];
    std::string logFile = argv[4];

    // create logger
    Logger logger(logFile);

    DNSServer server(mode, port, serverFile, &logger);
    server.start();

    return 0;
}
