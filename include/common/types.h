#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <string>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>

#include "helper.h"

struct NetworkInterface {
    std::string name;
    std::string ip_address;
    std::string mac_address;
    std::string netmask;
    std::string subnet_mask;
    bool is_ipv4;
    bool is_active;

    static NetworkInterface from_ifaddrs(struct ifaddrs* ifa);
};

struct NetworkNeighbor {
    std::string ip_address;
    std::string mac_address;
    std::string interface_name;
    int last_seen;
    bool is_active;
    bool is_ipv4;
};

#endif // COMMON_TYPES_H