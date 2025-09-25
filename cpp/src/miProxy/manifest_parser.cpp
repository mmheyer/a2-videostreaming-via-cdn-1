#include "manifest_parser.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// Helper function to extract attribute values from an XML tag
std::string get_attribute_value(const std::string& tag, const std::string& attribute_name) {
    std::string attribute = attribute_name + "=\"";
    size_t start_pos = tag.find(attribute);
    if (start_pos != std::string::npos) {
        start_pos += attribute.length();
        size_t end_pos = tag.find("\"", start_pos);
        if (end_pos != std::string::npos) {
            return tag.substr(start_pos, end_pos - start_pos);
        }
    }
    return "";
}

// Parses the MPEG-DASH manifest (.mpd) file content and extracts the available bitrates in Kbps
std::vector<int> parse_available_bitrates(const std::string& manifest_content) {
    std::vector<int> bitrates;

    size_t pos = 0;
    std::string media_tag = "<Representation";  // Looking for <media> elements

    // Loop through the manifest content and find each <media> tag
    while ((pos = manifest_content.find(media_tag, pos)) != std::string::npos) {
        // Find the end of the <media> tag
        size_t end_pos = manifest_content.find(">", pos);
        if (end_pos != std::string::npos) {
            std::string tag = manifest_content.substr(pos, end_pos - pos + 1);

            // Extract the bitrate attribute
            std::string bitrate_str = get_attribute_value(tag, "bandwidth");
            if (!bitrate_str.empty()) {
                try {
                    // int bitrate = std::stoi(bitrate_str) / 1000;  // Convert from bps to Kbps
                    bitrates.push_back(std::stoi(bitrate_str));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Invalid bitrate value in manifest: " << bitrate_str << std::endl;
                }
            }

            // Move the position forward to continue searching
            pos = end_pos + 1;
        } else {
            break;
        }
    }

    // Sort the bitrates in ascending order (optional)
    std::sort(bitrates.begin(), bitrates.end());

    return bitrates;
}
