#include "Proxy.hpp"
#include <iostream>
#include "spdlog/spdlog.h"

// Constructor (updated)
ClientConnection::ClientConnection(const std::string& manifest_path)
    : current_throughput(0.0), manifest_path(manifest_path) {}

// Getter for manifest path
const std::string& ClientConnection::getManifestPath() const {
    return manifest_path;
}

// Setter for manifest path
void ClientConnection::setManifestPath(const std::string& path) {
    manifest_path = path;
}

// Get the server IP address
// const std::string& ClientConnection::getServerIp() const {
//     return server_ip;
// }

// Get the current throughput
double ClientConnection::getCurrentThroughput() const {
    return current_throughput;
}

// Update the moving average throughput using EWMA
void ClientConnection::updateThroughput(double new_throughput, double alpha) {
    current_throughput = alpha * new_throughput + (1 - alpha) * current_throughput;
}

int ClientConnection::selectBitrate(Proxy& proxy) const {
    // Use the proxy's BitrateManager to get the available bitrates for the current manifest path
    const std::vector<int>* bitrates = proxy.getBitrateManager().getBitrates(manifest_path);
    if (bitrates == nullptr) {
        // If no bitrates are found, return 0
        return 0;
    }

    // Iterate over the available bitrates in descending order to find the highest supported bitrate
    for (auto rit = bitrates->rbegin(); rit != bitrates->rend(); ++rit) {
        if (current_throughput >= 1.5 * (*rit)) {
            return *rit;
        }
    }

    // If no suitable bitrate is found, return the lowest one
    return bitrates->front();
}

// Add a new client connection
void ConnectionManager::addClient(int client_fd) {
    client_map.emplace(client_fd, ClientConnection(""));
}

// Update the throughput for a specific client
void ConnectionManager::updateClientThroughput(int client_fd, double new_throughput, double alpha) {
    auto it = client_map.find(client_fd);
    if (it != client_map.end()) {
        it->second.updateThroughput(new_throughput, alpha);
    }
}

// int ConnectionManager::getClientWebSock(int client_fd) const {
//     return client_map.at(client_fd).getWebSock();
// }

// void ConnectionManager::setClientWebSockfd(int client_fd, int webSockfds) {
//     client_map.at(client_fd).setWebSock(webSockfds);
// }

// Get a const reference to a client connection (returns nullptr if not found)
const ClientConnection* ConnectionManager::getClient(int client_fd) const {
    auto it = client_map.find(client_fd);
    if (it != client_map.end()) {
        return &it->second;
    }
    return nullptr;
}

// Get a non-const reference to a client connection (returns nullptr if not found)
ClientConnection* ConnectionManager::getClient(int client_fd) {
    auto it = client_map.find(client_fd);
    if (it != client_map.end()) {
        return &it->second;
    }
    return nullptr;
}

// Remove a client connection
void ConnectionManager::removeClient(int client_fd) {
    client_map.erase(client_fd);
    spdlog::debug("Removed client {}", client_fd);
}

// int ClientConnection::getWebSock() const {
//     return web_sock;
// }

// void ClientConnection::setWebSock(int webSockfd) {
//     web_sock = webSockfd;
// }

const std::map<int, ClientConnection> & ConnectionManager::getClientMap() const {
    return client_map;
}

size_t ConnectionManager::getNumClients() {
    return client_map.size();
}
