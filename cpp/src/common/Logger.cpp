#include "Logger.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>  // For formatting floating-point numbers
#include "spdlog/spdlog.h"

// these are the new files :)

// Constructor
Logger::Logger(std::string &log_path) {
    log_file.open(log_path, std::ios::out);
    if (!log_file.is_open()) {
        std::cerr << "Error: Could not open log file: " << log_path << std::endl;
    }
}
// Destructor
Logger::~Logger() {
    log_file.close();
    spdlog::debug("Log file closed!");
}

// Logs a generic message (for startup events, errors, etc.)
bool Logger::log_message(const std::string& message) {
    // log_file << message << std::endl;
    spdlog::info(message);
    return true;
}
// Logs chunk download activity in the format: 
// <browser-ip> <chunkname> <server-ip> <duration> <tput> <avg-tput> <bitrate>
bool Logger::log_chunk_transfer(const std::string& browser_ip, const std::string& chunkname, const std::string& server_ip,
                        double duration, double tput, double avg_tput, int bitrate) {
    // log_file << browser_ip << " "
    //          << chunkname << " "
    //          << server_ip << " "
    //          << std::fixed << std::setprecision(3) << duration << " "  // Duration in seconds, 3 decimal places
    //          << std::fixed << std::setprecision(2) << tput << " "      // Throughput in Kbps, 2 decimal places
    //          << avg_tput << " "                                        // Average throughput (EWMA) in Kbps
    //          << bitrate << std::endl;                                  // Requested bitrate in Kbps
    spdlog::info("{} {} {} {:.3f} {:.2f} {:.2f} {}", browser_ip, chunkname, server_ip, duration, tput, avg_tput, bitrate);
    return true;
}

// Logs chunk download activity in the format: 
// <browser-ip> <chunkname> <server-ip> <duration> <tput> <avg-tput> <bitrate>
void Logger::log_dns_query(const std::string& client_ip, const std::string& query_name, const std::string& response_ip) {
    // log_file << client_ip << " " << query_name << " " << response_ip << std::endl;
    spdlog::info("{} {} {}", client_ip, query_name, response_ip);
}
