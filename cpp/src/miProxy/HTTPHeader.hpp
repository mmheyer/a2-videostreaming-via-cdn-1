#pragma once

#include <string>
#include <map>

namespace miProxy {

class HTTPHeader {
public:
	using HeaderMap = std::map<std::string, std::string>;

	HTTPHeader() = default;
	HTTPHeader(HeaderMap headers) : headers_(std::move(headers)) {}

	// Parse a block of headers (CRLF-separated lines) and populate the map.
	// Implementation provided in HTTPHeader.cpp.
	void parseFromString(const std::string& rawHeaders);

	// Parse a single header line "Name: value" and insert/replace entry.
	// Implementation provided in HTTPHeader.cpp.
	void parseFromLine(const std::string& line);

	// Serialize all headers back to "Name: value\r\n" lines (no trailing CRLF after block).
	// Implementation provided in HTTPHeader.cpp.
	std::string toString() const;

	// Accessors / mutators
	bool hasHeader(const std::string& name) const;
	std::string getHeader(const std::string& name, const std::string& defaultValue = "") const;
	void setHeader(const std::string& name, const std::string& value);
	void removeHeader(const std::string& name);

	const HeaderMap& headers() const { return headers_; }
	HeaderMap& headers() { return headers_; }

private:
	HeaderMap headers_;
};

} // namespace miProxy