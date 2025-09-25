#ifndef BITRATE_MANAGER_HPP
#define BITRATE_MANAGER_HPP

#include <map>
#include <vector>
#include <string>
#include <optional>

// Calculates throughput based on chunk size and duration
double calculate_throughput(size_t chunk_size, double duration);

class BitrateManager {
public:
    // Add or update the bitrates for a given manifest path
    void addBitrates(const std::string& manifest_path, const std::vector<int>& bitrates);

    // Retrieve the bitrates for a given manifest path
    const std::vector<int>* getBitrates(const std::string& manifest_path) const;

    // Remove the bitrates for a given manifest path
    void removeBitrates(const std::string& manifest_path);

    // Clear all stored bitrates
    void clear();

private:
    // Map to store available bitrates for each manifest path
    std::map<std::string, std::vector<int>> available_bitrate_map;
};

#endif  // BITRATE_MANAGER_HPP
