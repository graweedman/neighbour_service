#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <string>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>
#include <array>
#include <algorithm>
#include <time.h>
#include <vector>

typedef std::string MAC_Address; 
typedef std::string IP_Address;

#include "node_id.h"

struct NetworkInterface {
    std::string name;
    IP_Address ip_address;
    MAC_Address mac_address;
    IP_Address subnet_mask;
    std::string network_cidr;
    IP_Address broadcast_address;
    bool is_ipv4;
    bool is_active;

    static NetworkInterface from_ifaddrs(struct ifaddrs* ifa);
};

struct NetworkConnection {
    IP_Address ip;
    MAC_Address mac_address;
};

struct NetworkNeighbor {
    std::vector<std::string> interface_names;
    std::vector<NetworkConnection> connections;
    int last_seen;

    void update_last_seen();
    bool is_active() const;
    void add_interface(const NetworkInterface& interface);
    void add_connection(const NetworkConnection& connection);
};

struct DiscoveryPackage {
    std::string hello_message;
    NodeIDHex sender_id;
    IP_Address sender_ip;
    MAC_Address sender_mac;

    static DiscoveryPackage from_string(std::string& data);

private:
    static std::string extract_field(const std::string& message, const std::string& field_name);
};

#endif // COMMON_TYPES_H