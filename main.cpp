#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

using boost::asio::ip::udp;

const std::string MULTICAST_ADDRESS = "239.255.255.250";
const int SSDP_PORT = 2021;

struct Printer {
  std::string name;
  std::string usn;
  std::string model;
  std::string signal;
  std::string address;
};

std::string getCurrentTime() {
  std::time_t now = std::time(nullptr);
  char buf[80];
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT",
                std::gmtime(&now));
  return std::string(buf);
}

void loadConfig(const std::string &filePath, std::vector<Printer> &printers,
                std::string &listenAddress) {
  YAML::Node config = YAML::LoadFile(filePath);
  listenAddress = config["listen_address"].as<std::string>();
  YAML::Node printersNode = config["printers"];
  for (auto it = printersNode.begin(); it != printersNode.end(); ++it) {
    Printer printer;
    printer.name = it->first.as<std::string>();
    printer.usn = it->second["usn"].as<std::string>();
    printer.model = it->second["model"].as<std::string>();
    printer.signal = it->second["signal"].as<std::string>();
    printer.address = it->second["address"].as<std::string>();
    printers.push_back(printer);
  }
}

std::vector<std::string> generatePackets(const std::vector<Printer> &printers) {
  std::vector<std::string> packets;
  for (const auto &printer : printers) {
    std::string packet = "HTTP/1.1 200 OK\r\n"
                         "Server: Buildroot/2018.02-rc3 UPnP/1.0 ssdpd/1.8\r\n"
                         "Date: " +
                         getCurrentTime() +
                         "\r\n"
                         "Location: " +
                         printer.address +
                         "\r\n"
                         "ST: urn:bambulab-com:device:3dprinter:1\r\n"
                         "EXT:\r\n"
                         "USN: " +
                         printer.usn +
                         "\r\n"
                         "Cache-Control: max-age=1800\r\n"
                         "DevModel.bambu.com: " +
                         printer.model +
                         "\r\n"
                         "DevName.bambu.com: " +
                         printer.name +
                         "\r\n"
                         "DevSignal.bambu.com: " +
                         printer.signal +
                         "\r\n"
                         "DevConnect.bambu.com: lan\r\n"
                         "DevBind.bambu.com: free\r\n\r\n";
    packets.push_back(packet);
  }
  return packets;
}

void sendPackets(const std::vector<std::string> &packets,
                 const boost::asio::ip::address receiverAddress,
                 unsigned short receiverPort) {
  try {
    boost::asio::io_context ioContext;
    udp::socket socket(ioContext, udp::endpoint(udp::v4(), 0));
    udp::endpoint receiverEndpoint(receiverAddress, receiverPort);
    for (const auto &packet : packets) {
      socket.send_to(boost::asio::buffer(packet), receiverEndpoint);
      // std::cout << "Sent packet: " << packet << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Error sending packets: " << e.what() << "\n";
  }
}

int main(int argc, char *argv[]) {
  std::vector<Printer> printers;
  std::string listenAddress;
  std::string configFile = "config.yaml";
  if (argc > 1) {
    configFile = argv[1];
  }
  loadConfig(configFile, printers, listenAddress);
  auto packets = generatePackets(printers);
  try {
    boost::asio::io_service io_service;
    boost::asio::ip::address multicast_address =
        boost::asio::ip::address::from_string(MULTICAST_ADDRESS);
    boost::asio::ip::address listen_address =
        boost::asio::ip::address::from_string(listenAddress);
    udp::socket socket(io_service);
    udp::endpoint listen_endpoint(multicast_address, SSDP_PORT);
    socket.open(listen_endpoint.protocol());
    socket.set_option(udp::socket::reuse_address(true));
    socket.bind(listen_endpoint);
    socket.set_option(boost::asio::ip::multicast::join_group(
        multicast_address.to_v4(), listen_address.to_v4()));

    std::cout << "Listening for SSDP packets on multicast address "
              << MULTICAST_ADDRESS << " port " << SSDP_PORT << "...\n";

    while (true) {
      boost::array<char, 1024> recv_buf;
      udp::endpoint sender_endpoint;
      size_t len =
          socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
      std::string received_packet(recv_buf.data(), len);
      // std::cout << "Received packet:\n" << received_packet << "\n";
      if (received_packet.find("M-SEARCH * HTTP/1.1") != std::string::npos &&
          received_packet.find("urn:bambulab-com:device:3dprinter:1") !=
              std::string::npos) {
        // std::cout << "M-SEARCH request detected from " <<
        // sender_endpoint.address().to_string()
        //	<< ":" << sender_endpoint.port() << "\n";
        sendPackets(packets, sender_endpoint.address(), SSDP_PORT);
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  return 0;
}
