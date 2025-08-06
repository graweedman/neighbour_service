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

    std::string subnet_mask(const std::string& ip, int cidr) {
        struct in_addr address;
        inet_aton(ip.c_str(), &address);
        
        uint32_t mask = (0xFFFFFFFF << (32 - cidr)) & 0xFFFFFFFF;
        uint32_t network = ntohl(address.s_addr) & mask;

        struct in_addr network_address;
        network_address.s_addr = htonl(network);

        return std::string(inet_ntoa(network_address)) + "/" + std::to_string(cidr);
    }

    std::string get_mac_address(const std::string& interface_name) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        struct ifreq ifr;
        strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
        ioctl(sockfd, SIOCGIFHWADDR, &ifr);
        close(sockfd);

        char mac[18];
        strcpy(mac, ether_ntoa((struct ether_addr*)ifr.ifr_hwaddr.sa_data));

        return std::string(mac);
    }
} // namespace helper

