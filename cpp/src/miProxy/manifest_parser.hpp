#ifndef MANIFEST_PARSER_HPP
#define MANIFEST_PARSER_HPP

#include <string>
#include <vector>

// Helper function to extract attribute values from an XML tag
std::string get_attribute_value(const std::string& tag, const std::string& attribute_name);

// Parses the MPEG-DASH manifest (.mpd) file content and returns the available bitrates in Kbps
std::vector<int> parse_available_bitrates(const std::string& manifest_content);

#endif  // MANIFEST_PARSER_HPP
