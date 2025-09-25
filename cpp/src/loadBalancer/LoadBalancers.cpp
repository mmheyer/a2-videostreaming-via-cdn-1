#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <queue>
#include <climits>
#include "LoadBalancers.h"
#include "spdlog/spdlog.h"

// Ctor for RoundRobinLoadBalancer
RoundRobinLoadBalancer::RoundRobinLoadBalancer(const std::string &filename) : currentIndex(0) {
    std::ifstream file(filename);
    std::string ip;
    while (file >> ip) {
        // Add to the servers map using the index as the key
        servers[currentIndex] = ip;
        currentIndex++;
    }
    currentIndex = 0; // Reset index for round-robin use
}

// Get next server using round-robin algorithm
std::string RoundRobinLoadBalancer::getNextServer(const std::string &clientIP) {
    // Ignore clientIP
    std::string ip = servers[currentIndex]; // Get server IP from the servers map
    currentIndex = (currentIndex + 1) % static_cast<int>(servers.size()); // Update index for next call
    return ip;
}

// Ctor for GeoLoadBalancer
GeoLoadBalancer::GeoLoadBalancer(const std::string &filename) {
    loadNetwork(filename);
}

// Load the network of clients and servers from a file
void GeoLoadBalancer::loadNetwork(const std::string &filename) {
    std::ifstream file(filename);
    std::string numString;
    file >> numString >> numNodes;

    // Load nodes
    for (int i = 0; i < numNodes; i++) {
        int nodeId;
        std::string type, ip;
        file >> nodeId >> type >> ip;
        if (type == "CLIENT") {
            clients[nodeId] = ip; // Populate the clients map
        } else if (type == "SERVER") {
            servers[nodeId] = ip; // Populate the servers map
        }
    }

    // Load links
    int numLinks;
    file >> numString >> numLinks;

    for (int i = 0; i < numLinks; i++) {
        int origin, dest, cost;
        file >> origin >> dest >> cost;
        adjList[origin].emplace_back(dest, cost);
        adjList[dest].emplace_back(origin, cost);
    }

    for (auto &origin : adjList) {
        for (auto &dest : origin.second) {
            spdlog::debug("Cost of {} -> {} = {}", origin.first, dest.first, dest.second);
        }
    }
}

// Get the closest server to the client by performing Dijkstra's algorithm
int GeoLoadBalancer::getClosestServer(int clientId) {
    std::vector<int> dist(numNodes, INT_MAX);
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> pq;
    //TODO: Review Dijkstra's algo and see if it's doing what we want it to do

    dist[clientId] = 0;
    pq.push({0, clientId});

    while (!pq.empty()) {
        std::pair<int, int> top_element = pq.top();
        int d = top_element.first;
        int node = top_element.second;
        pq.pop();

        // If this node is a server, return it
        // TODO: check logic
        if (servers.find(node) != servers.end()) {
            return node;  // Return the first server found
        }

        // Explore neighbors
        for (std::pair<int, int> &current_node : adjList[node]) {
            int neighbor = current_node.first;
            int cost = current_node.second;
            int newDist = d + cost;
            if (newDist < dist[neighbor]) {
                dist[neighbor] = newDist;
                pq.push({newDist, neighbor});
            }
        }
    }

    return -1;  // If no server is found
}

// Get the closest server IP based on the client's IP address
std::string GeoLoadBalancer::getNextServer(const std::string &clientIP) {
    int clientId = -1;

    // Find the client ID based on the provided client IP
    for (const auto &client : clients) {
        spdlog::debug("Checking client with ID {} and IP {}", client.first, client.second);
        if (client.second == clientIP) {
            clientId = client.first;
            break;
        }
    }

    // If no client matches the IP, return an empty string
    if (clientId == -1) {
        return "[DEBUG] No client matches the IP";
    }

    // Get the closest server's ID
    int closestServerId = getClosestServer(clientId);

    // If no server is found, return an empty string
    if (closestServerId == -1) {
        return "[DEBUG] No server was found";
    }

    // Return the closest server's IP address
    return servers[closestServerId]; // Use the servers map to return the IP
}
