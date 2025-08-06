#include "common/types.h"

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
        interface.netmask = inet_ntoa(netmask_in->sin_addr);
        int cidr = helper::netmask_to_cidr(netmask_in);
        interface.subnet_mask = helper::subnet_mask(interface.ip_address, cidr);
    }

    return interface;
}