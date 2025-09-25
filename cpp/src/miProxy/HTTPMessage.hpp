#pragma once

#include <string>
#include <map>

namespace miProxy {

class HTTPMessage {
public:
	using HeaderMap = std::map<std::string, std::string>;

	HTTPMessage() = default;
	virtual ~HTTPMessage() = default;

	// Parse a block of header lines (CRLF-separated) and populate headers_.
	// Implementation provided in HTTPMessage.cpp.
	void parseHeaders(const std::string& rawHeaders);

	// Split a single header line "Name: value" into pair{name, value}.
	// Implementation provided in HTTPMessage.cpp.
	static std::pair<std::string, std::string> splitHeaderLine(const std::string& line);

	// Check for presence of a header (case-insensitive lookup will be performed by implementation).
	bool hasHeader(const std::string& name) const;

	// Get header value or defaultValue if not present.
	std::string getHeader(const std::string& name, const std::string& defaultValue = "") const;

	// Accessors for the whole header map (read-only).
	const HeaderMap& headers() const { return headers_; }

	// Body accessors.
	const std::string& body() const { return body_; }
	void setBody(const std::string& b) { body_ = b; }

	// The start-line (e.g., request-line or status-line) as parsed/stored by derived classes.
	const std::string& startLine() const { return startLine_; }

	// Derived classes must implement parsing of the start-line and serialization to string.
	virtual void parseStartLine(const std::string& startLine) = 0;
	virtual std::string toString() const = 0;

protected:
	HeaderMap headers_;
	std::string body_;
	std::string startLine_;
};

} // namespace miProxy