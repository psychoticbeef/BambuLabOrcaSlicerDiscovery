#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

using boost::asio::ip::udp;

namespace Config {
const std::string MULTICAST_ADDRESS = "239.255.255.250";
const unsigned short SSDP_PORT = 2021;
} // namespace Config

struct Printer {
  std::string name;
  std::string usn;
  std::string model;
  std::string signal;
  std::string address;
};

std::string getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t_now), "%a, %d %b %Y %H:%M:%S GMT");
  return oss.str();
}

void loadPrintersFromConfig(const std::string &filePath,
                            std::vector<Printer> &printers,
                            std::string &listenAddress) {
  YAML::Node config = YAML::LoadFile(filePath);
  listenAddress = config["listen_address"].as<std::string>();
  for (const auto &node : config["printers"]) {
    printers.push_back({node.first.as<std::string>(),
                        node.second["usn"].as<std::string>(),
                        node.second["model"].as<std::string>(),
                        node.second["signal"].as<std::string>(),
                        node.second["address"].as<std::string>()});
  }
}

std::string createPacket(const Printer &printer) {
  std::ostringstream packetStream;
  packetStream << "HTTP/1.1 200 OK\r\n"
               << "Server: Buildroot/2018.02-rc3 UPnP/1.0 ssdpd/1.8\r\n"
               << "Date: " << getCurrentTime() << "\r\n"
               << "Location: " << printer.address << "\r\n"
               << "ST: urn:bambulab-com:device:3dprinter:1\r\n"
               << "EXT:\r\n"
               << "USN: " << printer.usn << "\r\n"
               << "Cache-Control: max-age=1800\r\n"
               << "DevModel.bambu.com: " << printer.model << "\r\n"
               << "DevName.bambu.com: " << printer.name << "\r\n"
               << "DevSignal.bambu.com: " << printer.signal << "\r\n"
               << "DevConnect.bambu.com: lan\r\n"
               << "DevBind.bambu.com: free\r\n"
               << "\r\n";
  return packetStream.str();
}

std::vector<std::string> createPackets(const std::vector<Printer> &printers) {
  std::vector<std::string> packets;
  for (const auto &printer : printers) {
    packets.push_back(createPacket(printer));
  }
  return packets;
}

void sendPackets(const std::vector<std::string> &packets,
                 const udp::endpoint &receiverEndpoint) {
  try {
    boost::asio::io_context ioContext;
    udp::socket socket(ioContext, udp::endpoint(udp::v4(), 0));

    for (const auto &packet : packets) {
      socket.send_to(boost::asio::buffer(packet), receiverEndpoint);
      std::cout << "Sent packet: " << packet << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Error sending packets: " << e.what() << "\n";
  }
}

void handleIncomingPackets(udp::socket &socket,
                           const std::vector<std::string> &packets) {
  std::array<char, 1024> recv_buf;
  udp::endpoint senderEndpoint;

  while (true) {
    size_t len =
        socket.receive_from(boost::asio::buffer(recv_buf), senderEndpoint);
    std::string receivedPacket(recv_buf.data(), len);

    if (receivedPacket.find("M-SEARCH * HTTP/1.1") != std::string::npos &&
        receivedPacket.find("urn:bambulab-com:device:3dprinter:1") !=
            std::string::npos) {
      std::cout << "M-SEARCH request detected from "
                << senderEndpoint.address().to_string() << ":"
                << senderEndpoint.port() << "\n";
      sendPackets(packets, senderEndpoint);
    }
  }
}

void listenForSSDP(const std::string &listenAddress,
                   const std::vector<std::string> &packets) {
  try {
    boost::asio::io_context ioContext;
    udp::socket socket(ioContext);
    udp::endpoint listenEndpoint(
        boost::asio::ip::address::from_string(Config::MULTICAST_ADDRESS),
        Config::SSDP_PORT);

    socket.open(listenEndpoint.protocol());
    socket.set_option(udp::socket::reuse_address(true));
    socket.bind(listenEndpoint);
    socket.set_option(boost::asio::ip::multicast::join_group(
        boost::asio::ip::address::from_string(Config::MULTICAST_ADDRESS)
            .to_v4(),
        boost::asio::ip::address::from_string(listenAddress).to_v4()));

    std::cout << "Listening for SSDP packets on multicast address "
              << Config::MULTICAST_ADDRESS << " port " << Config::SSDP_PORT
              << "...\n";

    handleIncomingPackets(socket, packets);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

int main(int argc, char *argv[]) {
  std::vector<Printer> printers;
  std::string listenAddress;
  std::string configFile = (argc > 1) ? argv[1] : "config.yaml";

  loadPrintersFromConfig(configFile, printers, listenAddress);
  auto packets = createPackets(printers);

  listenForSSDP(listenAddress, packets);

  return 0;
}
