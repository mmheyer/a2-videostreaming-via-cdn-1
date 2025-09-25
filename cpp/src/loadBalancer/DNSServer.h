#ifndef __DNS_SERVER_H__
#define __DNS_SERVER_H__
#include "LoadBalancers.h"
#include <string>
#include "DNSHeader.h"
//#include "DNSQuestion.h"
//#include "DNSRecord.h"
#include "Logger.hpp"

// testing git again

class DNSServer {
public:
    DNSServer(const std::string &mode, int port, const std::string &serverFile, Logger *logger);
    ~DNSServer();
    // Added since last submit
    DNSHeader prepareResponse(ushort headerId, int rcode);
    void start();

private:
    int port;
    std::string mode;
    std::string serverFile;
    std::string logFile;
    int sockfd;
    LoadBalancer *loadBalancer; // Use a generic pointer for the load balancer
    Logger *logger;
};

#endif