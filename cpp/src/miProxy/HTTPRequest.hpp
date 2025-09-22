#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

class HTTPRequest {
private:
    // std::string header;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
public:
    // constructor
    HTTPRequest(std::string& header);

    // returns the full HTTP request as a string
    std::string rebuildRequest() const;

    // getters
    std::string getHeader(const std::string& headerName) const;
    std::string getChunkName() const;

    // setters (returns the full modified request)
    std::string setHeader(const std::string& headerName, const std::string& headerValue);
    std::string setBitrate(int new_bitrate);
};

#endif  // HTTP_REQUEST_HPP
