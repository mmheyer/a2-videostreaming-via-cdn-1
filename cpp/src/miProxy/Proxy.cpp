#include "Proxy.hpp"
#include "common.hpp"
#include "http_handler.hpp"
#include "Connection.hpp"
#include "manifest_parser.hpp"
#include "Logger.hpp"
#include <array>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>

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

// // Create a listening socket
// int Proxy::createListeningSocket() {
//     int sockfd;
//     struct sockaddr_in server_addr;

//     // Create socket
//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0) {
//         std::cerr << "Error creating socket\n";
//         exit(EXIT_FAILURE);
//     }

//     // Set up the server address structure
//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(static_cast<uint16_t>(listen_port));

//     // Bind the socket to the port
//     if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         std::cerr << "Error binding socket\n";
//         close(sockfd);
//         exit(EXIT_FAILURE);
//     }

//     // Listen for incoming connections
//     if (listen(sockfd, 10) < 0) {
//         std::cerr << "Error listening on socket\n";
//         close(sockfd);
//         exit(EXIT_FAILURE);
//     }

//     return sockfd;
// }

// Add a new client and open new web socket
void Proxy::addNewClient(int client_fd) {
    connection_manager.addClient(client_fd);
    std::cout << "New client added: " << client_fd << std::endl;
}

// Remove a client
void Proxy::removeClient(int client_fd) {
    connection_manager.removeClient(client_fd);
    std::cout << "Client removed: " << client_fd << std::endl;
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
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // set master socket to allow multiple connections ,
  // this is just a good habit, it will work without this
  int success =
      setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (success < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  address->sin_family = AF_INET;
  address->sin_addr.s_addr = INADDR_ANY;
  address->sin_port = htons(static_cast<uint16_t>(listen_port));

  // bind the socket to localhost port 8888
  success = ::bind(master_socket, (struct sockaddr *)address, sizeof(*address));
  if (success < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  printf("---Listening on port %d---\n", listen_port);

  // try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0) {
    perror("listen");
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
        // log_message("[DEBUG] Failed to open web socket.", log_path);
        std::cout << "[DEBUG] Failed to open web socket." << std::endl;
    }
    return web_sock;
}

// Handle a client request
void Proxy::handleClientRequest(int client_sock, std::string &request) {
    // Parse the URI from the HTTP GET request
    ssize_t bytes_read = -1;
    std::string uri = get_http_uri(request);
    size_t content_length = -1;

    std::cout << "[DEBUG] Handling URI: " << uri << std::endl;

    // Case 1: Handling manifest file requests (ends with ".mpd")
    if (uri.find(".mpd") != std::string::npos) {
        std::string original_manifest_uri = uri;
        std::cout << "[DEBUG] Into manifest case." << std::endl;

        // Fetch the original manifest file from the web server
        // std::string manifest_request = construct_http_get_request(original_manifest_uri, server_ip);
        std::string manifest_request = request;
        ssize_t bytes_sent = send(web_sock, manifest_request.c_str(), manifest_request.size(), 0);

        std::cout << "[DEBUG] Sent " << bytes_sent << " bytes." << std::endl;

        // receive the HTTP response header from the server
        std::string header;
        if (!read_http_header(web_sock, header)) 
            std::cout << "[DEBUG] Failed to retrieve HTTP header for manifest." << std::endl;
        std::cout << "[DEBUG] Header for manifest file: " << header << std::endl;

        // get the content length from the header
        content_length = get_content_length(header);

        // create the buffer with size content_length + 1
        // char buffer[content_length + 1];
        std::vector<char> buffer(content_length + 1);

        // receive content from server
        ssize_t bytes_read = recv(web_sock, buffer.data(), content_length, MSG_WAITALL);
        std::string manifest_content(buffer.data(), bytes_read);
        std::cout << "[DEBUG] Manifest content: " << manifest_content << std::endl;

        // Parse and store bitrates
        std::vector<int> bitrates = parse_available_bitrates(manifest_content);
        for (int rate : bitrates) std::cout << "[DEBUG] Bitrate = " << rate << std::endl;
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

        std::cout << "[DEBUG] No list manifest uri = " << no_list_manifest_uri << std::endl;

        // std::string no_list_manifest_request = construct_http_get_request(no_list_manifest_uri, server_ip);
        std::string no_list_manifest_request = modify_request_uri(manifest_request, no_list_manifest_uri);
        std::cout << "[DEBUG] No list manifest request = " << no_list_manifest_request << std::endl;
        send(web_sock, no_list_manifest_request.c_str(), no_list_manifest_request.size(), 0);

        // receive the HTTP response header from the server
        std::string no_list_header;
        if (!read_http_header(web_sock, no_list_header)) 
            std::cout << "[DEBUG] Failed to retrieve HTTP header for manifest no list." << std::endl;
        std::cout << "[DEBUG] No list manifest header: " << no_list_header << std::endl;

        // get the content length from the header
        content_length = get_content_length(no_list_header);

        // create the buffer with size content_length + 1
        // char no_list_buffer[content_length + 1];
        std::vector<char> no_list_buffer(content_length + 1);

        // get no list manifest contest
        bytes_read = recv(web_sock, no_list_buffer.data(), content_length, MSG_WAITALL);
        std::string no_list_manifest_content(no_list_buffer.data(), bytes_read);
        std::cout << "[DEBUG] No list manifest content = " << no_list_manifest_content << std::endl;

        // concatenate the no list header and no list content
        std::string no_list_msg = no_list_header + no_list_manifest_content;

        // Send the modified manifest to the client
        send(client_sock, no_list_msg.c_str(), no_list_msg.size(), 0);

    // Case 2: Handling video chunk requests (ends with ".m4s")
    } else if (uri.find(".m4s") != std::string::npos) {
        ClientConnection* client = connection_manager.getClient(client_sock);
        if (client) {
            // get highest bitrate supported based on current throughput
            int selected_bitrate = client->selectBitrate(*this);

            // modify URI to contain correct bitrate
            std::string modified_uri = modify_uri_bitrate(uri, selected_bitrate);
            
            // edit request to contain updated URI
            std::string modified_request = modify_request_uri(request, modified_uri);

            std::cout << "[DEBUG] Modified Request: " << modified_request << std::endl;

            // forward the request to the client
            send(web_sock, modified_request.c_str(), modified_request.size(), 0);

            TimePoint start_time = get_current_time();

            // receive the HTTP response header from the server
            std::string header;
            if (!read_http_header(web_sock, header)) 
                std::cout << "[DEBUG] Failed to retrieve HTTP header for video." << std::endl;

            // get the content length from the header
            content_length = get_content_length(header);

            // create the buffer with size content_length + 1
            // char buffer[content_length + 1];
            std::vector<char> buffer(content_length);

            // receive content from server
            bytes_read = recv(web_sock, buffer.data(), content_length, MSG_WAITALL);
            TimePoint end_time = get_current_time();

            if (bytes_read <= 0) {
                std::cerr << "[DEBUG] Error receiving video chunk data: " << strerror(errno) << std::endl;
                return;
            }

            std::cout << "[DEBUG] Received " << bytes_read << " bytes of video data from server." << std::endl;

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

            // store content in a string
            // std::string videoChunk(buffer.data(), bytes_read);

            // concatenate header and content
            // std::string msg = header + videoChunk;

            // first, send the header
            ssize_t header_sent = send(client_sock, header.c_str(), header.size(), 0);
            if (header_sent == -1) {
                std::cerr << "[DEBUG] Error sending HTTP header to client: " << strerror(errno) << std::endl;
                return;
            }

            // now, send the video chunk (binary data)
            ssize_t chunk_sent = send(client_sock, buffer.data(), bytes_read, 0);
            if (chunk_sent == -1) {
                std::cerr << "[DEBUG] Error sending video chunk to client: " << strerror(errno) << std::endl;
                return;
            }

            std::cout << "[DEBUG] Sent " << chunk_sent << " bytes of video chunk to client " << client_sock << std::endl;

            // send the http packet with the video chunk to the client
            // ssize_t bytes_sent = send(client_sock, msg.c_str(), msg.size(), 0);
            
            // error checking for send
            // if (bytes_sent == -1) {
            //     std::cerr << "[DEBUG] Error in sending video chunks to client: " << strerror(errno) << std::endl;
            // } else {
            //     std::cout << "[DEBUG] " << bytes_sent << " bytes of video sent to client " << client_sock << std::endl;
            // }
        }

    // Case 3: Handling requests for HTML, JavaScript, CSS, and other files
    } else {
        // Simply forward the request to the web server
        //std::string pass_through_request = construct_http_get_request(uri, server_ip);
        std::string pass_through_request = updateHostHeader(request, server_ip);
        std::cout << "[DEBUG] request = " << pass_through_request << std::endl;

        // forward client request to web server
        if (send(web_sock, pass_through_request.c_str(), pass_through_request.size(), 0) <= 0) {
            std::cout << "[DEBUG] Failed to pass request to web socket " << web_sock << std::endl;
        }
        std::cout << "[DEBUG] Passed request to web socket " << web_sock << std::endl;

        // receive the HTTP response header from the server
        std::string header;
        if (!read_http_header(web_sock, header)) 
            std::cout << "[DEBUG] Failed to retrieve HTTP header for other file." << std::endl;
        std::cout << "[DEBUG] Header of response from server: " << header << std::endl;

        // get the content length from the header
        content_length = get_content_length(header);
        std::cout << "[DEBUG] Content length = " << content_length << std::endl;

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
            std::cout << "[DEBUG] Bytes read: " << bytes_read << std::endl;
            // std::cout << "[DEBUG] Content: " << buffer << std::endl;
        } else if (bytes_read == 0) {
            std::cout << "[DEBUG] Connection closed by the server." << std::endl;
        } else {
            std::cerr << "[DEBUG] Error in recv: " << strerror(errno) << std::endl;
        }

        // std::string request(buffer, valread);
        std::cout << "[DEBUG] Buffer before send = " << buffer.data() << std::endl;

        // store the content in a string
        std::string content(buffer.data(), bytes_read);

        // concatenate the header and content
        std::string message = header + content;
        
        // send
        ssize_t bytes_sent = send(client_sock, message.c_str(), message.size(), 0);

        // size_t total_sent = 0;
        // ssize_t bytes_sent;

        // while (total_sent < bytes_read) {
        //     bytes_sent = send(client_sock, buffer + total_sent, bytes_read - total_sent, 0);
        //     if (bytes_sent == -1) {
        //         std::cerr << "Error sending data." << std::endl;
        //     }
        //     total_sent += bytes_sent;
        // }

        if (bytes_sent == -1) {
            std::cerr << "[DEBUG] Error in sending data to client: " << strerror(errno) << std::endl;
        } else {
            std::cout << "[DEBUG] " << bytes_sent << " bytes sent to client " << client_sock << std::endl;
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
        // for (int client_sock : client_sockets) {
        for (const auto& pair : connection_manager.getClientMap()) {
            int client_sock = pair.first;
            if (client_sock != 0) FD_SET(client_sock, &readfds);
            // if (client_sock > max_sd) {
            //     max_sd = client_sock;
            // }
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
            // if (new_sock >= 0) {
            //     // Add new client socket to the list of active client sockets
            //     // client_sockets.push_back(new_sock);
            //     addNewClient(new_sock);
            // }

            if (new_sock < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("\n---New host connection---\n");
            printf("socket fd is %d , ip is : %s , port : %d \n", new_sock,
                    inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // add new socket to client_map in the connection_manager
            // this function also opens a new web socket for the client
            addNewClient(new_sock);
            
            // // Create new socket to connect to the web server
            // int new_web_sock = socket(AF_INET, SOCK_STREAM, 0);

            // // Sets up the addr and port of the webserver
            // struct sockaddr_in web_addr;
            // memset(&web_addr, 0, sizeof(web_addr));
            // web_addr.sin_family = AF_INET;
            // web_addr.sin_port = htons(static_cast<uint16_t>(server_port));
            // inet_pton(AF_INET, server_ip.c_str(), &web_addr.sin_addr);
            // if (connect(new_web_sock, (struct sockaddr*)&web_addr, sizeof(web_addr)) < 0) {
            //     log_message("[DEBUG] Failed to open web socket.", log_path);
            // }

            // connection_manager.setClientWebSockfd(new_sock, new_web_sock);
        }

        // else it's some IO operation on a client socket
        std::cout << "[DEBUG] Num client sockets = " << connection_manager.getNumClients() << std::endl;
        for (const auto& pair : connection_manager.getClientMap()) {
            int client_sock = pair.first;
            // If the client socket is valid and ready for reading
            // NOTE: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds)) {
                std::cout << "[DEBUG] Reading from client socket " << client_sock << std::endl;

                // Check if it was for closing
                getpeername(client_sock, (struct sockaddr *)&address,
                    (socklen_t *)&addrlen);

                // read message from client
                valread = static_cast<int>(read(client_sock, buffer.data(), BUFFER_SIZE - 1));

                // get the web socket the client is connected to
                // int web_sock = connection_manager.getClientWebSock(client_sock);

                // // peek at the data without actually removing it from the socket stream
                // std::cout << "[DEBUG] In buffer before receiving new data = " << buffer << std::endl;
                // ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer), MSG_PEEK);

                if (valread == 0) {
                    // Somebody disconnected , get their details and print
                    printf("\n---Host disconnected---\n");
                    printf("Host disconnected , ip %s , port %d \n",
                            inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    
                    // Close the sockets and remove from map
                    close(client_sock);
                    // close(web_sock);
                    connection_manager.removeClient(client_sock);
                } else {
                    // convert buffer to a C++ string (no need for null termination)
                    std::string request(buffer.data(), valread);
                    std::cout << "Received request: " << request << std::endl;
                    handleClientRequest(client_sock, request);
                }

                // if (bytes_read > 0) {
                //     // Data received from the client, handle it
                //     // buffer[bytes_read] = '\0';  // Null-terminate the buffer if it's a string
                //     // std::cout << "Received data: " << buffer << std::endl;

                //     // Check if it was for closing, and also read the incoming message
                //     std::string header = "";
                //     if (read_http_header(client_sock, header) <= 0) {
                //         std::cout << "[DEBUG] No bytes read into header." << std::endl;
                //     }

                //     // handle the client request accordingly
                //     handleClientRequest(client_sock, web_sock, header);
                // } else if (bytes_read == 0) {
                //     // Connection closed by the client
                //     std::cout << "[DEBUG] Client closed the connection." << std::endl;
                //     close(client_sock);
                //     close(web_sock);
                //     connection_manager.removeClient(client_sock);
                // } else {
                //     // An error occurred
                //     std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                //     if (errno == ECONNRESET) {
                //         // Connection reset by peer
                //         std::cout << "Connection reset by the client." << std::endl;
                //     }
                //     close(client_sock);
                //     close(web_sock);
                // }
            }
        }
    }
}