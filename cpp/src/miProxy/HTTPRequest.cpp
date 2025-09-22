#include "http_request.hpp"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <cctype>
#include "spdlog/spdlog.h"

// Constructor
HTTPRequest::HTTPRequest(std::string& header)  {
    // Helper lambdas
    auto trim = [](std::string &s) {
        // left trim
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        // right trim
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    };

    auto to_lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        return s;
    };

    // Split header into lines. HTTP header section is separated from body by "\r\n\r\n".
    size_t pos = header.find("\r\n\r\n");
    std::string header_section = header;
    if (pos != std::string::npos) {
        header_section = header.substr(0, pos);
        body = header.substr(pos + 4); // store any body that followed
    }

    std::istringstream stream(header_section);
    std::string line;

    // First line is the request/status line. Store it under a pseudo-header key "request-line".
    if (std::getline(stream, line)) {
        // remove possible trailing '\r'
        if (!line.empty() && line.back() == '\r') line.pop_back();
        trim(line);
        headers["request-line"] = line;
    }

    // Parse remaining header lines of the form "Name: value"
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; // skip blank lines

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue; // malformed line

        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        trim(name);
        trim(value);
        // normalize header name to lowercase for easier lookup
        headers[to_lower(name)] = value;
    }
}


// Rebuild the full HTTP request as a string
std::string HTTPRequest::rebuildRequest() const {
    std::ostringstream stream;
    // Write the request/status line
    auto it = headers.find("request-line");
    if (it != headers.end()) {
        stream << it->second << "\r\n";
    }

    // Write all other headers
    for (const auto& pair : headers) {
        if (pair.first == "request-line") continue; // already written
        stream << pair.first << ": " << pair.second << "\r\n";
    }

    // End of headers
    stream << "\r\n";

    // Append body if present
    if (!body.empty()) {
        stream << body;
    }

    return stream.str();
}


// Getters

// Helper function to find the Content-Length header and return its value
std::string HTTPRequest::getHeader(const std::string& headerName) const {
    // try headers map first
    auto it = headers.find(headerName);
    if (it != headers.end()) {
        return it->second;
    }
    spdlog::debug("Header {} not found", headerName);
    return "";
}

// Extracts the chunk filename from the URI (for logging purposes)
std::string HTTPRequest::getChunkName() const {
    // Example: /path/to/video/vid-500-seg-2.m4s
    std::string uri = getHeader("request-uri");
    size_t last_slash = uri.find_last_of("/");
    if (last_slash != std::string::npos) {
        return uri.substr(last_slash + 1);  // Return the filename part
    }
    return ""; // return empty if no slash found
}


// Setters
// Modifies or adds a header in the HTTP request
std::string HTTPRequest::setHeader(const std::string& headerName, const std::string& headerValue) {
    headers[headerName] = headerValue;
    return rebuildRequest();
}

// Modifies the requested URI to adjust the bitrate in the request
std::string HTTPRequest::setBitrate(int new_bitrate) {
    // Find the position of the bitrate within the URI
    // std::string uri = get_http_uri();
    std::string uri = getHeader("request-uri");
    size_t bitrate_pos = uri.find("-seg-");
    if (bitrate_pos != std::string::npos) {
        // Find the previous dash to locate where the bitrate starts
        size_t bitrate_start = uri.rfind("-", bitrate_pos - 1);
        if (bitrate_start != std::string::npos) {
            // Replace the bitrate in the URI with the new bitrate
            uri.replace(bitrate_start + 1, bitrate_pos - bitrate_start - 1, std::to_string(new_bitrate));
        }
    }
    headers["request-uri"] = uri;
    return rebuildRequest();
}
