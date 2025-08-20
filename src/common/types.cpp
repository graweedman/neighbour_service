#include "common/types.h"
#include "common/helper.h"

NetworkInterface NetworkInterface::from_ifaddrs(struct ifaddrs* ifa) {
    // Only handle IPv4 for this function
    if (ifa->ifa_addr->sa_family != AF_INET) { 
        return NetworkInterface();
    }

    NetworkInterface interface;
    interface.name = ifa->ifa_name;
    interface.is_active = (ifa->ifa_flags & IFF_UP) != 0;
    interface.is_ipv4 = true;

    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
    interface.ip_address = inet_ntoa(addr_in->sin_addr);
    interface.mac_address = helper::get_mac_address(interface.name);
    
    if (ifa->ifa_netmask) {
        struct sockaddr_in* netmask_in = (struct sockaddr_in*)ifa->ifa_netmask;
        interface.subnet_mask = inet_ntoa(netmask_in->sin_addr);
        int cidr = helper::netmask_to_cidr(netmask_in);
        interface.network_cidr = helper::network_cidr(interface.ip_address, cidr);
    }
    interface.broadcast_address = helper::broadcast_address(interface.network_cidr);

    return interface;
}

void NetworkNeighbor::update_last_seen() {
    last_seen = time(nullptr);
}

bool NetworkNeighbor::is_active() const {
    return (time(nullptr) - last_seen) < 30; // Active if seen in the last 30 seconds
}

void NetworkNeighbor::add_interface(const NetworkInterface& interface) {
    if (std::find(interface_names.begin(), interface_names.end(), interface.name) == interface_names.end()) {
        interface_names.push_back(interface.name);
    }
}

void NetworkNeighbor::add_connection(const NetworkConnection& connection) {
    auto it = std::find_if(connections.begin(), connections.end(),
        [&connection](const NetworkConnection& conn) { 
            return conn.mac_address == connection.mac_address; 
        });

    if (it != connections.end()) {
        it->ip = connection.ip; // Update IP if MAC already exists
    } else {
        connections.push_back(connection); // Add new connection
    }
}

DiscoveryPackage DiscoveryPackage::from_string(std::string& data) {
    DiscoveryPackage pkg;
    
    pkg.hello_message = data;

    size_t from_pos = data.find(" from ");
    if (from_pos != std::string::npos) {
        size_t name_start = from_pos + 6; // Skip " from "
        size_t name_end = data.find(" ", name_start);
        if (name_end != std::string::npos) {
            std::string interface_name = data.substr(name_start, name_end - name_start);
        }
    }
    
    // Extract NodeID (after "NodeID:")
    pkg.sender_id = extract_field(data, "NodeID:");
    
    // Extract MAC address (after "MAC:")
    pkg.sender_mac = extract_field(data, "MAC:");
    
    // Extract IP address (after "IP:")
    pkg.sender_ip = extract_field(data, "IP:");
    
    return pkg;
}

std::string DiscoveryPackage::extract_field(const std::string& message, const std::string& field_name) {
    size_t start_pos = message.find(field_name);
    if (start_pos == std::string::npos) return "";

    start_pos += field_name.length();
    size_t end_pos = message.find(" ", start_pos);
    if (end_pos == std::string::npos) end_pos = message.length();

    return message.substr(start_pos, end_pos - start_pos);
}