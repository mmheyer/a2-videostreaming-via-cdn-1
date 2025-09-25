#include <array>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include "spdlog/spdlog.h"

#include "Proxy.hpp"
#include "common.hpp"
#include "Connection.hpp"
#include "manifest_parser.hpp"
#include "Logger.hpp"
#include "HTTPMessage.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

#define MAXCLIENTS 30
/*
 *  Compile with: g++ --std=c++11 echo_server.cpp
 *  Try to run this server and run multiple instances
 *  of "nc localhost 8888" to communicate with it!
 *
 *  Derived from:
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 */

// Constructor
Proxy::Proxy(int listen_port, const std::string& server_ip, int server_port, double alpha, Logger &logger)
    : listen_port(listen_port), server_ip(server_ip), server_port(server_port), alpha(alpha), logger(logger) {
        // open web socket
        web_sock = openWebSock();
    }

// Destructor
Proxy::~Proxy() {
    close(web_sock);
}

// Add a new client and open new web socket
void Proxy::addNewClient(int client_fd) {
    connection_manager.addClient(client_fd);
    spdlog::debug("New client added: {}", client_fd);
}

// Remove a client
void Proxy::removeClient(int client_fd) {
    connection_manager.removeClient(client_fd);
    spdlog::debug("Client removed: {}", client_fd);
}

BitrateManager& Proxy::getBitrateManager() {
    return bitrate_manager;
}

int Proxy::getMasterSocket(struct sockaddr_in *address) {
  int yes = 1;
  int master_socket;
  // create a master socket
  master_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (master_socket <= 0) {
    spdlog::error("socket failed");
    exit(EXIT_FAILURE);
  }

  // set master socket to allow multiple connections ,
  // this is just a good habit, it will work without this
  int success =
      setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (success < 0) {
    spdlog::error("setsockopt");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  address->sin_family = AF_INET;
  address->sin_addr.s_addr = INADDR_ANY;
  address->sin_port = htons(static_cast<uint16_t>(listen_port));

  // bind the socket to localhost port 8888
  success = ::bind(master_socket, (struct sockaddr *)address, sizeof(*address));
  if (success < 0) {
    spdlog::error("bind failed");
    exit(EXIT_FAILURE);
  }
  printf("---Listening on port %d---\n", listen_port);

  // try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0) {
    spdlog::error("listen");
    exit(EXIT_FAILURE);
  }
  return master_socket;
}

int Proxy::openWebSock() {
    // Create new socket to connect to the web server
    int web_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Sets up the addr and port of the webserver
    struct sockaddr_in web_addr;
    memset(&web_addr, 0, sizeof(web_addr));
    web_addr.sin_family = AF_INET;
    web_addr.sin_port = htons(static_cast<uint16_t>(server_port));
    inet_pton(AF_INET, server_ip.c_str(), &web_addr.sin_addr);
    if (connect(web_sock, (struct sockaddr*)&web_addr, sizeof(web_addr)) < 0) {
        // log_message("Failed to open web socket.", log_path);
        spdlog::error("Failed to open web socket.");
    }
    return web_sock;
}

void Proxy::sendHTTPMessage(int sock, HTTPMessage &http_message) {
    std::string message = http_message.rebuildRequest();
    const char *data = message.c_str();
    size_t total_len = message.size();
    ssize_t total_sent = 0;

    while (total_sent < total_len) {
        ssize_t sent = send(sock, data + total_sent, total_len - total_sent, 0);
        if (sent == -1) {
            spdlog::error("Error sending HTTP request to socket {}: {}", sock, strerror(errno));
            return;
        }
        total_sent += sent;
    }
    spdlog::debug("Sent {} bytes to socket {}", total_sent, sock);
}

HTTPRequest Proxy::receiveHTTPRequest(int read_sock) {
    char byte;
    ssize_t bytes_read;
    std::string recent_chars;
    std::string header = "";

    // Read header byte-by-byte until "\r\n\r\n"
    while (true) {
        bytes_read = recv(read_sock, &byte, 1, 0);

        // Check if the client has closed the connection or an error occurred
        if (bytes_read <= 0) {
            spdlog::debug("{} bytes read. Client closed connection or error.", bytes_read);
            return HTTPRequest(); // return empty request on error
        }

        header.push_back(byte);

        // Maintain last-four-chars buffer
        recent_chars.push_back(byte);
        if (recent_chars.size() > 4) {
            recent_chars.erase(0, recent_chars.size() - 4);
        }

        if (recent_chars == "\r\n\r\n") {
            break; // end of header reached
        }
    }

    HTTPRequest http_request;
    http_request.parseHeader(header);

    // Determine content length from header (0 if absent or parsing fails)
    int content_length = stoi(http_request.getHeader("content-length"));

}

// Handle a client request
void Proxy::handleClientRequest(int client_sock) {
    // Parse the HTTP request
    // HTTPRequest http_request(client_sock);
    HTTPRequest http_request = receiveHTTPRequest(client_sock);

    // Parse the URI from the HTTP GET request
    ssize_t bytes_read = -1;
    std::string uri = http_request.getHeader("request-uri");
    int content_length = stoi(http_request.getHeader("content-length"));

    spdlog::debug("Handling URI: {}", uri);

    // Case 1: Handling manifest file requests (ends with ".mpd")
    if (uri.find(".mpd") != std::string::npos) {
        std::string original_manifest_uri = uri;

        // Fetch the original manifest file from the web server
        // std::string manifest_request = request;
        // sendToWebServer(http_request.rebuildRequest());
        sendHTTPRequest(web_sock, http_request);

        // spdlog::debug("Sent {} bytes.", bytes_sent);
        // std::string client_uuid = get_uuid_from_request(request);
        std::string client_uuid = http_request.getHeader("x-489-uuid");
        spdlog::info("Manifest requested by {} forwarded to {}:{} for {}", client_uuid, server_ip, server_port, uri);

        // get the content length from the header
        // content_length = get_content_length(header);

        // create the buffer with size content_length + 1
        // char buffer[content_length + 1];
        std::vector<char> buffer(content_length + 1);

        // receive content from server
        ssize_t bytes_read = recv(web_sock, buffer.data(), content_length, MSG_WAITALL);
        std::string manifest_content(buffer.data(), bytes_read);
        spdlog::debug("Manifest content: {}", manifest_content);

        // Parse and store bitrates
        std::vector<int> bitrates = parse_available_bitrates(manifest_content);
        for (int rate : bitrates) spdlog::debug("Bitrate = {}", rate);
        bitrate_manager.addBitrates(original_manifest_uri, bitrates);

        // Set manifest path in the ClientConnection
        ClientConnection* client = connection_manager.getClient(client_sock);
        if (client) {
            client->setManifestPath(original_manifest_uri);
        }

        // Construct and fetch the "-no-list.mpd" version for the client
        std::string no_list_manifest_uri = original_manifest_uri;
        size_t dot_pos = no_list_manifest_uri.find_last_of('.');
        if (dot_pos != std::string::npos) {
            no_list_manifest_uri.insert(dot_pos, "-no-list");
        }

        spdlog::debug("No list manifest uri = {}", no_list_manifest_uri);

        // std::string no_list_manifest_request = construct_http_get_request(no_list_manifest_uri, server_ip);
        // std::string no_list_manifest_request = set_uri(manifest_request, no_list_manifest_uri);
        std::string no_list_manifest_request = http_request.setHeader("request-uri", no_list_manifest_uri);
        spdlog::debug("No list manifest request = {}", no_list_manifest_request);
        // sendToWebServer(no_list_manifest_request);
        sendHTTPRequest(web_sock, HTTPRequest(no_list_manifest_request));

        // receive the HTTP response header from the server
        std::string no_list_header;
        if (!read_http_header(web_sock, no_list_header))
            spdlog::debug("Failed to retrieve HTTP header for manifest no list.");
        spdlog::debug("No list manifest header: {}", no_list_header);

        // get the content length from the header
        content_length = get_content_length(no_list_header);

        // create the buffer with size content_length + 1
        // char no_list_buffer[content_length + 1];
        std::vector<char> no_list_buffer(content_length + 1);

        // get no list manifest contest
        bytes_read = recv(web_sock, no_list_buffer.data(), content_length, MSG_WAITALL);
        std::string no_list_manifest_content(no_list_buffer.data(), bytes_read);
        spdlog::debug("No list manifest content = {}", no_list_manifest_content);

        // concatenate the no list header and no list content
        std::string no_list_msg = no_list_header + no_list_manifest_content;

        // Send the modified manifest to the client
        sendToClient(client_sock, no_list_msg);

    // Case 2: Handling video chunk requests (ends with ".m4s")
    } else if (uri.find(".m4s") != std::string::npos) {
        ClientConnection* client = connection_manager.getClient(client_sock);
        if (client) {
            // get highest bitrate supported based on current throughput
            int selected_bitrate = client->selectBitrate(*this);

            // modify URI to contain correct bitrate
            std::string modified_uri = setBitrate(uri, selected_bitrate);
            
            // edit request to contain updated URI
            std::string modified_request = set_uri(request, modified_uri);

            spdlog::debug("Modified Request: {}", modified_request);

            // forward the request to the web server
            sendToWebServer(modified_request);

            std::string client_uuid = get_uuid_from_request(request);
            spdlog::info("Segment requested by {} forwarded to {}:{} as {} at bitrate {} Kbps", client_uuid, server_ip, server_port, uri, selected_bitrate); 

            TimePoint start_time = get_current_time();

            // receive the HTTP response header from the server
            std::string header;
            if (!read_http_header(web_sock, header))
                spdlog::debug("Failed to retrieve HTTP header for video.");

            // get the content length from the header
            content_length = get_content_length(header);

            // create the buffer with size content_length + 1
            // char buffer[content_length + 1];
            std::vector<char> buffer(content_length);

            // receive content from server
            bytes_read = recv(web_sock, buffer.data(), content_length, MSG_WAITALL);
            TimePoint end_time = get_current_time();

            if (bytes_read <= 0) {
                std::cerr << "Error receiving video chunk data: " << strerror(errno) << std::endl;
                return;
            }

            spdlog::debug("Received {} bytes of video data from server.", bytes_read);

            // log throughput and other metrics
            double duration = calculate_duration(start_time, end_time);
            double new_throughput = calculate_throughput(bytes_read, duration);
            connection_manager.updateClientThroughput(client_sock, new_throughput, alpha);

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            getpeername(client_sock, (struct sockaddr*)&client_addr, &client_len);
            std::string browser_ip = inet_ntoa(client_addr.sin_addr);
            std::string chunkname = extract_chunk_name(modified_uri);
            logger.log_chunk_transfer(browser_ip, chunkname, server_ip, duration, new_throughput, client->getCurrentThroughput(), selected_bitrate);

            // first, send the header
            sendToClient(client_sock, header);

            // now, send the video chunk (binary data)
            ssize_t chunk_sent = sendToClient(client_sock, std::string(buffer.data(), bytes_read));

            spdlog::debug("Sent {} bytes of video chunk to client {}", chunk_sent, client_sock);
        }

    
    } 
    // Case 3: POST requests to /on-fragment-received
    else if {
        // TODO: Handle POST requests to /on-fragment-received
    }
    // Case 4: Handling requests for HTML, JavaScript, CSS, and other files
    else {
        // Simply forward the request to the web server
        //std::string pass_through_request = construct_http_get_request(uri, server_ip);
        std::string pass_through_request = set_host(request, server_ip);
        spdlog::debug("request = {}", pass_through_request);

        // forward client request to web server
        sendToWebServer(pass_through_request);

        // receive the HTTP response header from the server
        std::string header;
        if (!read_http_header(web_sock, header))
            spdlog::debug("Failed to retrieve HTTP header for other file.");
        spdlog::debug("Header of response from server: {}", header);

        // get the content length from the header
        content_length = get_content_length(header);
        spdlog::debug("Content length = {}", content_length);

        // create the buffer with size content_length + 1
        // char buffer[content_length + 1];
        std::vector<char> buffer(content_length + 1);

        // receive the actual content from server
        bytes_read = recv(web_sock, buffer.data(), content_length, MSG_WAITALL);

        // Check if recv read any bytes
        if (bytes_read > 0) {
            // Null-terminate the buffer to safely print it as a string
            buffer[bytes_read] = '\0';
            
            // Print the number of bytes read and the content
            spdlog::debug("Bytes read: {}", bytes_read);
            // std::cout << "Content: " << buffer << std::endl;
        } else if (bytes_read == 0) {
            spdlog::debug("Connection closed by the server.");
        } else {
            std::cerr << "Error in recv: " << strerror(errno) << std::endl;
        }

        // std::string request(buffer, valread);
        spdlog::debug("Buffer before send = {}", buffer.data());

        // store the content in a string
        std::string content(buffer.data(), bytes_read);

        // concatenate the header and content
        std::string message = header + content;
        
        // send
        ssize_t bytes_sent = sendToClient(client_sock, message);
        if (bytes_sent == -1) {
            std::cerr << "Error in sending data to client: " << strerror(errno) << std::endl;
        } else {
            spdlog::debug("{} bytes sent to client {}", bytes_sent, client_sock);
        }
    }
}

// Main method to run the proxy
void Proxy::run() {
    int master_socket, addrlen, activity, valread;

    struct sockaddr_in address;
    master_socket = getMasterSocket(&address);

    // char buffer[BUFFER_SIZE]; // data buffer of 1KiB + 1 bytes
    std::vector<char> buffer(BUFFER_SIZE);

    // accept the incoming connection
    addrlen = sizeof(address);

    // set of socket descriptors
    fd_set readfds;

    while (true) {
        // Clear the set of fds (sockets) that the select()
        FD_ZERO(&readfds);

        // Adds the listening socket to the set of fds (listen_sock is responsible for accepting
        // new client connections)
        FD_SET(master_socket, &readfds);

        // Tracks the highest socket fd for use with select(), initially set to listen_sock
        // int max_sd = master_socket;

        // Adds each connected client's socket to readfds and updates max_sd to ensure select() 
        // monitors the highest socket number
        for (const auto& pair : connection_manager.getClientMap()) {
            int client_sock = pair.first;
            if (client_sock != 0) FD_SET(client_sock, &readfds);
        }

        // Blocks until there is activity or an error. 
        // int activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);
        activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            std::cout << "Error in select()\n";
        }

        // Checks if the listening socket has activity, meaning a new client is trying to connect
        if (FD_ISSET(master_socket, &readfds)) {
            // Accepts new client connection
            int new_sock = accept(master_socket, (struct sockaddr *)&address,
                              (socklen_t *)&addrlen);

            if (new_sock < 0) {
                spdlog::error("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            spdlog::info("New client socket connected with {}:{} on sockfd {}", inet_ntoa(address.sin_addr), ntohs(address.sin_port), new_sock);

            // add new socket to client_map in the connection_manager
            // this function also opens a new web socket for the client
            addNewClient(new_sock);
        }

        // else it's some IO operation on a client socket
        std::cout << "Num client sockets = " << connection_manager.getNumClients() << std::endl;
        for (const auto& pair : connection_manager.getClientMap()) {
            int client_sock = pair.first;
            // If the client socket is valid and ready for reading
            // NOTE: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds)) {
                std::cout << "Reading from client socket " << client_sock << std::endl;

                // Check if it was for closing
                getpeername(client_sock, (struct sockaddr *)&address,
                    (socklen_t *)&addrlen);

                // read message from client
                valread = static_cast<int>(read(client_sock, buffer.data(), BUFFER_SIZE - 1));

                if (valread == 0) {
                    // Somebody disconnected , get their details and print
                    spdlog::info("Client socket sockfd {} disconnected", client_sock); 
                    
                    // Close the sockets and remove from map
                    close(client_sock);
                    connection_manager.removeClient(client_sock);
                } else {
                    // convert buffer to a C++ string (no need for null termination)
                    std::string request(buffer.data(), valread);
                    std::cout << "Received request: " << request << std::endl;
                    handleClientRequest(client_sock);
                }
            }
        }
    }
}