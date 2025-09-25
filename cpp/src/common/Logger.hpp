#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

class Logger {
private:
    std::ofstream log_file;

public:
    // Constructor
    Logger(std::string &log_path);

    // Destructor
    ~Logger();

    // Logs a generic message (for startup events, errors, etc.)
    bool log_message(const std::string& message);

    // Logs chunk download activity with the specified details
    bool log_chunk_transfer(const std::string& browser_ip, const std::string& chunkname, const std::string& server_ip,
                            double duration, double tput, double avg_tput, int bitrate);

    // Logs chunk download activity in the format: 
    // <browser-ip> <chunkname> <server-ip> <duration> <tput> <avg-tput> <bitrate>
    void log_dns_query(const std::string& client_ip, const std::string& query_name, const std::string& response_ip);
};

#endif  // LOGGER_HPP
