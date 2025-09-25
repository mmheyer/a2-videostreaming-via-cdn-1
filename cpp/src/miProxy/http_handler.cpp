#include "http_handler.hpp"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <cctype>

// Helper function to find the Content-Length header and return its value
size_t get_content_length(const std::string& response) {
    size_t content_length = 0;
    size_t pos = response.find("Content-Length: ");
    if (pos != std::string::npos) {
        // Find the end of the Content-Length line
        size_t end_pos = response.find("\r\n", pos);
        if (end_pos != std::string::npos) {
            // Extract the number after "Content-Length: "
            std::string content_length_str = response.substr(pos + 16, end_pos - (pos + 16));
            content_length = std::stoi(content_length_str);
        }
    }
    return content_length;
}

// Parses the HTTP request and returns the requested URI
// The parsing is simplified to extract only the URI
std::string get_http_uri(const std::string& request) {
    std::cout << "[DEBUG] Original request = " << request << std::endl;
    size_t uri_start = request.find("GET ") + 4;
    size_t uri_end = request.find(" ", uri_start);

    // Return the URI (e.g., /path/to/resource)
    if (uri_start != std::string::npos && uri_end != std::string::npos) {
        return request.substr(uri_start, uri_end - uri_start);
    }
    
    return "";  // Return empty if parsing fails
}

ssize_t read_http_header(int read_sock, std::string &header) {
    char byte;
    ssize_t bytes_read;
    std::string recent_chars;
    while (true) {
        // Read a single byte from the socket
        bytes_read = read(read_sock, &byte, 1);

        // Check if the client has closed the connection or an error occurred
        if (bytes_read <= 0) {
            std::cout << "[DEBUG] " << bytes_read << " bytes read. Client closed connection." << std::endl; 
            return -1;
        }

        // Append the byte to the header string
        header += byte;
        // std::cout << "[DEBUG] Current header = " << header << std::endl;

        // Update the recent characters buffer
        recent_chars += byte;
        if (recent_chars.size() > 4) {
            recent_chars.erase(0, 1); // Keep only the last four characters
        }

        // Check if the last four characters are "\r\n\r\n"
        if (recent_chars == "\r\n\r\n") {
            return header.size(); // End of HTTP header reached
        }
    }

    return 0;
}

// Function to extract the video name from the URI
std::string extract_video_name(const std::string& uri) {
    // Find the position of the last slash
    size_t last_slash_pos = uri.find_last_of('/');
    if (last_slash_pos == std::string::npos) {
        // No slash found, return an empty string
        return "";
    }

    // Find the first dash after the last slash
    size_t dash_pos = uri.find('-', last_slash_pos);
    if (dash_pos == std::string::npos) {
        // No dash found, return an empty string
        return "";
    }

    // Extract the substring between the last slash and the first dash
    return uri.substr(last_slash_pos + 1, dash_pos - last_slash_pos - 1);
}

// Modifies the requested URI to adjust the bitrate in the request
std::string modify_uri_bitrate(const std::string& uri, int new_bitrate) {
    std::string modified_uri = uri;

    // Find the position of the bitrate within the URI
    size_t bitrate_pos = modified_uri.find("-seg-");
    if (bitrate_pos != std::string::npos) {
        // Find the previous dash to locate where the bitrate starts
        size_t bitrate_start = modified_uri.rfind("-", bitrate_pos - 1);
        if (bitrate_start != std::string::npos) {
            // Replace the bitrate in the URI with the new bitrate
            modified_uri.replace(bitrate_start + 1, bitrate_pos - bitrate_start - 1, std::to_string(new_bitrate));
        }
    }

    return modified_uri;
}

std::string modify_request_uri(const std::string& original_request, const std::string& new_uri) {
    // Find the end of the request line (first line of the HTTP request)
    size_t request_line_end = original_request.find("\r\n");
    if (request_line_end == std::string::npos) {
        // Return the original request if no request line is found
        return original_request;
    }

    // Extract the request line
    std::string request_line = original_request.substr(0, request_line_end);
    std::istringstream request_stream(request_line);
    std::string method, uri, http_version;

    // Parse the request line into method, URI, and HTTP version
    if (!(request_stream >> method >> uri >> http_version)) {
        // If parsing fails, return the original request
        return original_request;
    }

    // Construct the new request line with the modified URI
    std::string new_request_line = method + " " + new_uri + " " + http_version;

    // Reconstruct the modified request with the new request line
    std::string modified_request = new_request_line + original_request.substr(request_line_end);

    return modified_request;
}

// // Constructs an HTTP GET request with the modified URI and host
// std::string construct_http_get_request(const std::string& uri, const std::string& host) {
//     std::ostringstream request;
//     request << "GET " << uri << " HTTP/1.1\r\n";
//     request << "Host: " << host << "\r\n";
//     request << "Connection: close\r\n";
//     request << "\r\n";  // Marks the end of the headers
//     return request.str();
// }

// Function to update the Host header in an HTTP request
std::string updateHostHeader(const std::string& originalRequest, const std::string& newHost) {
    std::istringstream requestStream(originalRequest);
    std::string line;
    std::string updatedRequest;
    bool hostUpdated = false;

    // Process each line of the original request
    while (std::getline(requestStream, line)) {
        // Find and replace the Host header
        if (line.find("Host:") != std::string::npos && !hostUpdated) {
            updatedRequest += "Host: " + newHost + "\r\n";
            hostUpdated = true;
        } else {
            updatedRequest += line + "\n";  // Append other lines as is
        }
    }

    // If the Host header wasn't found, add it
    if (!hostUpdated) {
        updatedRequest += "Host: " + newHost + "\r\n";
    }

    return updatedRequest;
}

// Extracts the chunk filename from the URI (for logging purposes)
std::string extract_chunk_name(const std::string& uri) {
    // Example: /path/to/video/vid-500-seg-2.m4s
    size_t last_slash = uri.find_last_of("/");
    if (last_slash != std::string::npos) {
        return uri.substr(last_slash + 1);  // Return the filename part
    }
    return uri;
}

// Function to modify the Connection header in an HTTP request
std::string modify_connection_to_keep_alive(const std::string& original_request) {
    std::string modified_request = original_request;
    
    // Find the "Connection:" header in a case-insensitive manner
    std::string connection_header = "Connection:";
    auto pos = std::search(modified_request.begin(), modified_request.end(),
                           connection_header.begin(), connection_header.end(),
                           [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });

    // If "Connection:" header is found
    if (pos != modified_request.end()) {
        // Move the iterator to the end of "Connection:"
        pos += connection_header.size();

        // Skip any whitespace after "Connection:"
        while (pos != modified_request.end() && std::isspace(*pos)) {
            ++pos;
        }

        // Find the end of the current connection value (until the next line break)
        auto end_pos = std::find(pos, modified_request.end(), '\r');

        // Replace the value with "keep-alive"
        std::string new_value = "keep-alive";
        modified_request.replace(pos, end_pos, new_value);
    }

    return modified_request;
}
