#ifndef COMMON_HPP
#define COMMON_HPP

#include <chrono>
#include <string>

// --- Constants ---

// Common buffer size for reading data
constexpr size_t BUFFER_SIZE = 4096;  // 4 KB buffer

// Default alpha value for EWMA throughput calculation (you can remove this if it's passed dynamically)
constexpr double DEFAULT_ALPHA = 0.2;

// --- Time Handling ---

// Define TimePoint as a convenience alias for steady_clock time points
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

// Function to get the current time (for time measurement)
TimePoint get_current_time() {
    return std::chrono::steady_clock::now();
}

// Function to calculate the duration between two time points in seconds
double calculate_duration(const TimePoint& start, const TimePoint& end) {
    return std::chrono::duration<double>(end - start).count();  // Duration in seconds
}

// --- Utility Functions ---

// Utility function to trim whitespace from the start and end of a string (if needed)
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    
    return str.substr(start, end - start + 1);
}

#endif  // COMMON_HPP
