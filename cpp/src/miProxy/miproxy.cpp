#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <unistd.h>  // Required for close()
#include "Proxy.hpp"
#include "Logger.hpp"

// std::string get_server_IP(std::string &dns_ip, int dns_port) {
//     // TODO: finish for DNS mode
//     return "";
// }

// Function to print usage information in case of incorrect command-line arguments
void print_usage() {
    std::cerr << "Usage (Method 1 - No DNS): ./miProxy --nodns <listen-port> <www-ip> <alpha> <log>\n";
    std::cerr << "Usage (Method 2 - With DNS): ./miProxy --dns <listen-port> <dns-ip> <dns-port> <alpha> <log>\n";
}

int main(int argc, char* argv[]) {
    // Check if minimum number of arguments is provided
    if (argc < 6) {
        print_usage();
        return 1;
    }

    std::string mode = argv[1];
    int listen_port;
    double alpha;
    std::string log_path;
    std::string www_ip;

    if (mode == "--nodns") {
        if (argc != 6) {
            print_usage();
            return 1;
        }

        // Parse arguments for Method 1 (No DNS)
        listen_port = std::stoi(argv[2]);
        www_ip = argv[3];  // Web server IP
        alpha = std::stod(argv[4]);
        log_path = argv[5];

        // initialize logger
        Logger logger(log_path);
        // logger.log_message("MiProxy started in --nodns mode");

        // Log the IP of the web server we're connecting to
        // logger.log_message("Connecting to web server at IP: " + www_ip);

        // Create a Proxy instance and run it
        try {
            Proxy proxy(listen_port, www_ip, 80, alpha, logger);
            proxy.run();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            logger.log_message("Error: " + std::string(e.what()));
            return 1;
        }
    } else if (mode == "--dns") {
        if (argc != 7) {
            print_usage();
            return 1;
        }

        // Parse arguments for Method 2 (With DNS)
        listen_port = std::stoi(argv[2]);
        std::string dns_ip = argv[3];  // DNS server IP
        // int dns_port = std::stoi(argv[4]);
        alpha = std::stod(argv[5]);
        log_path = argv[6];

        // TODO: get server ip from DNS
        // www_ip = get_server_IP(dns_ip, dns_port);
        www_ip = "";

        // initialize logger
        Logger logger(log_path);
        // logger.log_message("MiProxy started in --dns mode");

        // Log the IP of the web server we're connecting to
        // logger.log_message("Connecting to web server at IP: " + www_ip);

        // Create a Proxy instance and run it
        try {
            Proxy proxy(listen_port, www_ip, 80, alpha, logger);
            proxy.run();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            logger.log_message("Error: " + std::string(e.what()));
            return 1;
        }
    } else {
        std::cerr << "Invalid mode specified. Use either --nodns or --dns.\n";
        print_usage();
        return 1;
    }

    return 0;
}
