#ifndef COMMON_HELPER_H
#define COMMON_HELPER_H

#include <ifaddrs.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <cstring>


#include "types.h"
namespace helper {

int netmask_to_cidr(struct sockaddr_in* netmask);
std::string subnet_mask(const std::string& ip, int cidr);
std::string get_mac_address(const std::string& interface_name);
// std::vector<NetworkInterface> get_network_interfaces();

} // namespace helper

#endif // COMMON_HELPER_H