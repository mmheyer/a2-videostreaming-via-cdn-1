#include "BitrateManager.hpp"

// Function to calculate throughput in Kbps
// chunk_size is in bytes and duration is in seconds
double calculate_throughput(size_t chunk_size, double duration) {
    // Convert chunk size to kilobits and duration to seconds to get Kbps
    return (static_cast<double>(chunk_size) * 8) / (duration * 1000);  // Return throughput in Kbps
}

// Add or update the bitrates for a given manifest path
void BitrateManager::addBitrates(const std::string& manifest_path, const std::vector<int>& bitrates) {
    available_bitrate_map[manifest_path] = bitrates;
}

// Retrieve the bitrates for a given manifest path
const std::vector<int>* BitrateManager::getBitrates(const std::string& manifest_path) const {
    auto it = available_bitrate_map.find(manifest_path);
    if (it != available_bitrate_map.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Remove the bitrates for a given manifest path
void BitrateManager::removeBitrates(const std::string& manifest_path) {
    available_bitrate_map.erase(manifest_path);
}

// Clear all stored bitrates
void BitrateManager::clear() {
    available_bitrate_map.clear();
}
