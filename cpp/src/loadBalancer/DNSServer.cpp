#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include "DNSServer.h"
#include "LoadBalancers.h" // Include LoadBalancer classes
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"
#include "Logger.hpp"
#include "spdlog/spdlog.h"

DNSServer::DNSServer(const std::string &mode, int port, const std::string &serverFile, Logger *logger)
    : port(port), mode(mode), serverFile(serverFile), logger(logger) {
    if (mode == "--rr") {
        loadBalancer = new RoundRobinLoadBalancer(serverFile);
    } else if (mode == "--geo") {
        loadBalancer = new GeoLoadBalancer(serverFile);
    }
}

DNSServer::~DNSServer() {
    delete loadBalancer;
} 

DNSHeader DNSServer::prepareResponse(ushort headerId, int rcode) {
    // Prepare the response
    DNSHeader responseHeader;
    responseHeader.AA = 1; // Authoritative Answer
    responseHeader.RD = 0;
    responseHeader.RA = 0; // Recursion Available
    responseHeader.Z = 0; // Reserved
    responseHeader.NSCOUNT = 0; // No authority records
    responseHeader.ARCOUNT = 0; // No additional records

    responseHeader.ID = headerId; // Copy the request ID
    responseHeader.QR = 1; // Set QR to 1 for response
    responseHeader.RCODE = 0; // No error
    responseHeader.QDCOUNT = 1; // One question
    responseHeader.ANCOUNT = 1; // One answer
    responseHeader.OPCODE = 0; // Standard Query
    responseHeader.TC = 0; // 0 For non-truncated msg
    
    if (rcode == 3) {
        responseHeader.RCODE = 3;
    }
    return responseHeader;
}

void DNSServer::start() {
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        spdlog::error("Failed to create socket");
        exit(1);
    }

    // Bind socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_int16_t>(port));
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        spdlog::error("Bind failed");
        close(sockfd);
        exit(1);
    }

    // Listen for connections
    listen(sockfd, 5);
    spdlog::info("Load balancer started on port {}", port);

    while (true) {
        int newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
        if (newsockfd < 0) {
            spdlog::error("Accept failed");
            continue;
        }

        // Process DNS queries
        // Receive DNS Header Size
        int headerSize;
        recv(newsockfd, &headerSize, sizeof(headerSize), 0);
        headerSize = ntohl(headerSize); // Convert from network byte order to host byte order

        // Receive DNS Header
        char* headerBuffer = new char[headerSize];
        recv(newsockfd, headerBuffer, headerSize, 0);
        DNSHeader header = DNSHeader::decode(std::string(headerBuffer, headerSize));
        delete[] headerBuffer;

        // Receive DNS Question Size
        int questionSize;
        recv(newsockfd, &questionSize, sizeof(questionSize), 0);
        questionSize = ntohl(questionSize);

        // Receive DNS Question
        char* questionBuffer = new char[questionSize];
        recv(newsockfd, questionBuffer, questionSize, 0);
        DNSQuestion question = DNSQuestion::decode(std::string(questionBuffer, questionSize));
        // Added since last submit
        question.QTYPE = 1;
        question.QCLASS = 1;
        delete[] questionBuffer;

        // Prepare response
        // Added since last submit
        std::string name = question.QNAME; // Queried name
        // Check if URL is "video.cse.umich.edu"
        int rcode = 0;
        if (name != "video.cse.umich.edu") {
            rcode = 3;
        }
        DNSHeader responseHeader = prepareResponse(header.ID, rcode);

        // Prepare a DNSRecord TODO: check
        DNSRecord record;
        record.TYPE = 1; // Type A
        record.CLASS = 1; // Class IN
        record.TTL = 0; // No caching
        //record.RDLENGTH = static_cast<u_int16_t>(strlen(record.RDATA));

        // RESOLVE QUESTION
        // Review QTYPE and QCLASS
        // Extract client IP address
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        spdlog::debug("Received connection from: {}", clientIP);

        // Resolve IP
        std::string ipAddress = loadBalancer->getNextServer(clientIP); // Use the client's IP to get the server
        //std::string name = question.QNAME; // Queried name
        strncpy(record.NAME, name.c_str(), sizeof(record.NAME) - 1); // Copy name safely
        record.NAME[sizeof(record.NAME) - 1] = '\0'; // Ensure null termination
        strncpy(record.RDATA, ipAddress.c_str(), sizeof(record.RDATA) - 1); // Copy RDATA safely
        record.RDATA[sizeof(record.RDATA) - 1] = '\0'; // Ensure null termination 
        // Added since last submit 
        record.RDLENGTH = htons(static_cast<u_int16_t>(ipAddress.size()));     

        // NAMESERVER LOGGING only if rcode == 0
        if (rcode == 0) {
            logger->log_dns_query(clientIP, name, ipAddress);
        }

        // Encode the response header and record
        std::string encodedHeader = DNSHeader::encode(responseHeader);
        std::string encodedRecord = DNSRecord::encode(record);

        // Send response header size
        int responseHeaderSize = htonl(static_cast<u_int32_t>(encodedHeader.size()));
        send(newsockfd, &responseHeaderSize, sizeof(responseHeaderSize), 0);

        // Send response header
        send(newsockfd, encodedHeader.c_str(), encodedHeader.size(), 0);

        // Only send record if rcode == 0
        if (rcode == 0) {
            // Send response record size
            int responseRecordSize = htonl(static_cast<u_int32_t>(encodedRecord.size()));
            send(newsockfd, &responseRecordSize, sizeof(responseRecordSize), 0);

            // Send response record
            send(newsockfd, encodedRecord.c_str(), encodedRecord.size(), 0);
        }

        // Close the connection
        close(newsockfd);
    }
}
