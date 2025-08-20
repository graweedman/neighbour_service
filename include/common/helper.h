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
#include <iostream>

#include "types.h"

namespace helper {

int netmask_to_cidr(struct sockaddr_in* netmask);
std::string network_cidr(const std::string& ip, int cidr);
IP_Address broadcast_address(const std::string& network_cidr);
MAC_Address get_mac_address(const std::string& interface_name);
bool is_ip_in_network(const std::string& ip, const std::string& network_cidr);
void log_error(const std::string& message, bool quiet_mode);
void log_info(const std::string& message, bool quiet_mode);

} // namespace helper

#endif // COMMON_HELPER_H