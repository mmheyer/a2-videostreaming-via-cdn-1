#ifndef PROXY_HPP
#define PROXY_HPP

#include <fstream>
#include <string>
#include <vector>
#include <optional>

#include "Connection.hpp"
#include "BitrateManager.hpp"
#include "Logger.hpp"
#include "HTTPMessage.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

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
    void handleClientRequest(int read_sock);
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

    // functions for sending messages to client/server from proxy
    void sendHTTPMessage(int sock, HTTPMessage &http_message);
    HTTPRequest receiveHTTPRequest(int read_sock);
    HTTPResponse receiveHTTPResponse(int read_sock);
};

#endif  // PROXY_HPP
