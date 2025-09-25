#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include <string>

size_t get_content_length(const std::string& response);

// Parses an HTTP request and returns the requested URI
std::string get_http_uri(const std::string& request);

// Function to extract the video name from the URI
std::string extract_video_name(const std::string& uri);

// Modifies the requested URI to adjust the bitrate in the request
std::string modify_uri_bitrate(const std::string& uri, int new_bitrate);

std::string modify_request_uri(const std::string& original_request, const std::string& new_uri);

std::string updateHostHeader(const std::string& originalRequest, const std::string& newHost);

// Constructs an HTTP GET request with the modified URI
std::string construct_http_get_request(const std::string& uri, const std::string& host);

ssize_t read_http_header(int read_sock, std::string &header);

// Handles manifest requests: fetches the manifest from the CDN and sends a modified one to the client
// std::string handle_manifest_request(const std::string& original_manifest_uri, const std::string& manifest_content);

// Extracts the chunk filename from the URI (for logging purposes)
std::string extract_chunk_name(const std::string& uri);

// Function to modify the Connection header in an HTTP request
std::string modify_connection_to_keep_alive(const std::string& original_request);

#endif  // HTTP_HANDLER_HPP
