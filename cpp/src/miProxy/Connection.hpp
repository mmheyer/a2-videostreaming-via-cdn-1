#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Proxy.hpp"
#include <map>
#include <string>
#include <vector>

class Proxy;

class ClientConnection {
public:
    // Constructor
    ClientConnection(const std::string& manifest_path);

    // Getters
    // const std::string& getServerIp() const;
    double getCurrentThroughput() const;

    // Update the moving average throughput
    void updateThroughput(double new_throughput, double alpha);

    // Select the highest supported bitrate based on current throughput
    int selectBitrate(Proxy& proxy) const;

    // Getters and setters for manifest path
    const std::string& getManifestPath() const;
    void setManifestPath(const std::string& path);

    // getters and setters for web_sockfd
    // int getWebSock() const;
    // void setWebSock(int webSockfds);

private:
    // std::string server_ip;          // IP address of the server the client is connected to
    double current_throughput;      // Current estimated throughput (moving average)
    std::string manifest_path;       // New member to store the manifest path
    // int web_sock;                   // Web socket that client is connected to
};

class ConnectionManager {
public:
    // Add a new client connection
    void addClient(int client_fd);

    // Update the throughput for a specific client
    void updateClientThroughput(int client_fd, double new_throughput, double alpha);

    const std::map<int, ClientConnection> &getClientMap() const;

    size_t getNumClients();

    // getters and setters for web_sockfd
    // int getClientWebSock(int client_fd) const;
    // void setClientWebSockfd(int client_fd, int webSockfds);

    // Get a client connection (const and non-const versions)
    const ClientConnection* getClient(int client_fd) const;
    ClientConnection* getClient(int client_fd);

    // Remove a client connection
    void removeClient(int client_fd);

private:
    std::map<int, ClientConnection> client_map; // Map of client file descriptor to ClientConnection
};

#endif  // CONNECTION_HPP