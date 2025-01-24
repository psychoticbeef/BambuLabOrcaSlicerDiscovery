#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <ctime>

using boost::asio::ip::udp;

// Global variables for configuration
std::string PRINTER_USN;
std::string PRINTER_MODEL;
std::string PRINTER_NAME;
std::string PRINTER_SIGNAL;
std::string LISTEN_ADDRESS;
std::string PRINTER_ADDRESS;
const std::string MULTICAST_ADDRESS = "239.255.255.250"; // SSDP multicast address
const int SSDP_PORT = 2021;                              // SSDP default port for discovery

std::string get_current_time() {
    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
    return std::string(buf);
}

void load_config(const std::string& config_file) {
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        PRINTER_USN = config["PRINTER_USN"].as<std::string>();
        PRINTER_MODEL = config["PRINTER_MODEL"].as<std::string>();
        PRINTER_NAME = config["PRINTER_NAME"].as<std::string>();
        PRINTER_SIGNAL = config["PRINTER_SIGNAL"].as<std::string>();
        LISTEN_ADDRESS = config["LISTEN_ADDRESS"].as<std::string>();
        PRINTER_ADDRESS = config["PRINTER_ADDRESS"].as<std::string>();
        std::cout << "Configuration loaded successfully from " << config_file << ".\n";
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}

void send_response(const std::string& sender_ip, const int destination_port) {
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Server: Buildroot/2018.02-rc3 UPnP/1.0 ssdpd/1.8\r\n"
        "Date: " + get_current_time() + "\r\n"
        "Location: " + PRINTER_ADDRESS + "\r\n"
        "ST: urn:bambulab-com:device:3dprinter:1\r\n"
        "EXT:\r\n"
        "USN: " + PRINTER_USN + "\r\n"
        "Cache-Control: max-age=1800\r\n"
        "DevModel.bambu.com: " + PRINTER_MODEL + "\r\n"
        "DevName.bambu.com: " + PRINTER_NAME + "\r\n"
        "DevSignal.bambu.com: " + PRINTER_SIGNAL + "\r\n"
        "DevConnect.bambu.com: lan\r\n"
        "DevBind.bambu.com: free\r\n\r\n";

    boost::asio::io_service io_service;
    udp::socket temp_socket(io_service, udp::endpoint(udp::v4(), 0)); // Bind to an ephemeral port
    udp::endpoint target_endpoint(boost::asio::ip::address::from_string(sender_ip), destination_port);
    temp_socket.send_to(boost::asio::buffer(response), target_endpoint);

    std::cout << "Response sent to " << sender_ip << ":" << destination_port << "\n";
    std::cout << "\n" << response << "\n";
}

int main() {
    load_config("config.yaml"); // Load configuration from YAML file

    try {
        boost::asio::io_service io_service;

        // Join the multicast group
        boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string(MULTICAST_ADDRESS);
        boost::asio::ip::address listen_address = boost::asio::ip::address::from_string(LISTEN_ADDRESS);
        udp::socket socket(io_service);
        udp::endpoint listen_endpoint(multicast_address, SSDP_PORT);
        socket.open(listen_endpoint.protocol());
        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket.bind(listen_endpoint);
        socket.set_option(boost::asio::ip::multicast::join_group(multicast_address.to_v4(), listen_address.to_v4()));

        std::cout << "Listening for SSDP packets on multicast address " << MULTICAST_ADDRESS << " port " << SSDP_PORT << "...\n";

        while (true) {
            boost::array<char, 1024> recv_buf;
            udp::endpoint sender_endpoint;

            size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);

            std::string received_packet(recv_buf.data(), len);
            std::cout << "Received packet:\n" << received_packet << "\n";

            if (received_packet.find("M-SEARCH * HTTP/1.1") != std::string::npos &&
                received_packet.find("urn:bambulab-com:device:3dprinter:1") != std::string::npos) {
                std::cout << "M-SEARCH request detected from " << sender_endpoint.address().to_string()
                          << ":" << sender_endpoint.port() << "\n";

                send_response(sender_endpoint.address().to_string(), SSDP_PORT);
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
