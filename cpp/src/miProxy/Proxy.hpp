#ifndef PROXY_HPP
#define PROXY_HPP

#include "Connection.hpp"
#include "BitrateManager.hpp"
#include "Logger.hpp"
#include <fstream>
#include <string>
#include <vector>

class Proxy {
public:
    // Constructor
    Proxy(int listen_port, const std::string& server_ip, int server_port, double alpha, Logger &logger);

    // Destructor
    ~Proxy();

    // Getters
    BitrateManager& getBitrateManager();

    // Main method to run the proxy
    void run();

private:
    // Helper methods
    void handleClientRequest(int client_sock, std::string &header);
    void addNewClient(int client_fd);
    void removeClient(int client_fd);

    // Member variables
    int listen_port;
    std::string server_ip;
    int server_port;
    double alpha;
    std::string log_path;
    int web_sock;
    Logger &logger;

    // Managers for connections and bitrates
    ConnectionManager connection_manager;
    BitrateManager bitrate_manager;

    // Method to create the listening socket
    // int createListeningSocket();

    // function from select discussion code
    int getMasterSocket(struct sockaddr_in *address);

    // open new connection to the web server
    int openWebSock();
};

#endif  // PROXY_HPP
