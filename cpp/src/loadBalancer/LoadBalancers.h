#ifndef __LOAD_BALANCERS_H__
#define __LOAD_BALANCERS_H__

#include <string>
#include <map>
#include <vector>

// Base class for Load Balancers
class LoadBalancer {
public:
    virtual ~LoadBalancer() = default; // Virtual destructor for proper cleanup
    virtual std::string getNextServer(const std::string &clientIP = "") = 0; // Pure virtual function

    // Public member variables
    std::map<int, std::string> clients;  // Client map
    std::map<int, std::string> servers;  // Server map
};

// Round-robin load balancer
class RoundRobinLoadBalancer : public LoadBalancer {
public:
    RoundRobinLoadBalancer(const std::string &filename);
    std::string getNextServer(const std::string &clientIP = "") override; // Override base class method

private:
    int currentIndex; // Keep track of the current server index
};

// Geographic load balancer
class GeoLoadBalancer : public LoadBalancer {
public:
    GeoLoadBalancer(const std::string &filename);
    std::string getNextServer(const std::string &clientIP) override; // Override base class method

private:
    void loadNetwork(const std::string &filename);
    int getClosestServer(int clientId);
    std::map<int, std::vector<std::pair<int, int>>> adjList; // Adjacency list for graph representation
    int numNodes; // Total number of nodes in the network
};

#endif
