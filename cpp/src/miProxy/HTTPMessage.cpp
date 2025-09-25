#include "HTTPMessage.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace miProxy {

namespace {
	// trim whitespace from both ends
	inline std::string trim(const std::string& s) {
		const std::string whitespace = " \t\r\n";
		const auto start = s.find_first_not_of(whitespace);
		if (start == std::string::npos) return "";
		const auto end = s.find_last_not_of(whitespace);
		return s.substr(start, end - start + 1);
	}

	inline std::string toLower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
		return s;
	}
} // namespace

void HTTPMessage::parseHeaders(const std::string& rawHeaders) {
	headers_.clear();
	std::istringstream ss(rawHeaders);
	std::string line;
	while (std::getline(ss, line)) {
		// Remove possible trailing '\r'
		if (!line.empty() && line.back() == '\r') line.pop_back();
		line = trim(line);
		if (line.empty()) continue;
		auto p = splitHeaderLine(line);
		std::string name = toLower(trim(p.first));
		std::string value = trim(p.second);
		if (!name.empty()) {
			// store with lower-case name to allow case-insensitive lookup
			headers_[name] = value;
		}
	}
}

std::pair<std::string, std::string> HTTPMessage::splitHeaderLine(const std::string& line) {
	auto pos = line.find(':');
	if (pos == std::string::npos) {
		// no colon -- treat whole line as name with empty value
		return { trim(line), std::string() };
	}
	std::string name = line.substr(0, pos);
	std::string value = line.substr(pos + 1);
	return { trim(name), trim(value) };
}

bool HTTPMessage::hasHeader(const std::string& name) const {
	std::string key = toLower(name);
	return headers_.find(key) != headers_.end();
}

std::string HTTPMessage::getHeader(const std::string& name, const std::string& defaultValue) const {
	std::string key = toLower(name);
	auto it = headers_.find(key);
	if (it == headers_.end()) return defaultValue;
	return it->second;
}

} // namespace miProxy