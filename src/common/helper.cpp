#include "common/helper.h"

namespace helper {
    int netmask_to_cidr(struct sockaddr_in* netmask) {
        if (!netmask) return 0;

        uint32_t mask = ntohl(netmask->sin_addr.s_addr);
        int cidr = 0;
        while (mask) {
            cidr += (mask & 1);
            mask >>= 1;
        }
        return cidr;
    }

    std::string network_cidr(const std::string& ip, int cidr) {
        struct in_addr address;
        inet_aton(ip.c_str(), &address);
        
        uint32_t mask = (0xFFFFFFFF << (32 - cidr)) & 0xFFFFFFFF;
        uint32_t network = ntohl(address.s_addr) & mask;

        struct in_addr network_address;
        network_address.s_addr = htonl(network);

        return std::string(inet_ntoa(network_address)) + "/" + std::to_string(cidr);
    }

    IP_Address broadcast_address(const std::string& network_cidr) {
        size_t pos = network_cidr.find('/');
        if (pos == std::string::npos) return "";

        std::string ip_part = network_cidr.substr(0, pos);
        int cidr = std::stoi(network_cidr.substr(pos + 1));

        struct in_addr address;
        inet_aton(ip_part.c_str(), &address);

        uint32_t mask = (0xFFFFFFFF << (32 - cidr)) & 0xFFFFFFFF;
        uint32_t broadcast = ntohl(address.s_addr) | ~mask;

        struct in_addr broadcast_address;
        broadcast_address.s_addr = htonl(broadcast);

        return IP_Address(inet_ntoa(broadcast_address));
    }

    MAC_Address get_mac_address(const std::string& interface_name) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        struct ifreq ifr;
        strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
        ioctl(sockfd, SIOCGIFHWADDR, &ifr);
        close(sockfd);

        char mac[18];
        strcpy(mac, ether_ntoa((struct ether_addr*)ifr.ifr_hwaddr.sa_data));

        return std::string(mac);
    }

    bool is_ip_in_network(const std::string &ip, const std::string &network_cidr)
    {
        size_t pos = network_cidr.find('/');
        if (pos == std::string::npos) return false;

        std::string network_ip = network_cidr.substr(0, pos);
        int cidr = std::stoi(network_cidr.substr(pos + 1));

        struct in_addr ip_addr, network_addr;
        inet_aton(ip.c_str(), &ip_addr);
        inet_aton(network_ip.c_str(), &network_addr);

        uint32_t mask = (0xFFFFFFFF << (32 - cidr)) & 0xFFFFFFFF;
        return (ntohl(ip_addr.s_addr) & mask) == (ntohl(network_addr.s_addr) & mask);
    }

    void log_error(const std::string& message, bool quiet_mode) {
        if (!quiet_mode) {
            std::cerr << "Error: " << message << std::endl;
        }
    }
    
    void log_info(const std::string& message, bool quiet_mode) {
        if (!quiet_mode) {
            std::cout << "Info: " << message << std::endl;
        }
    }
}
