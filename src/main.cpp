#include "main.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <string>
#include "common/types.h"
#include "common/helper.h"

using namespace std;

const int DISCOVERY_PORT = 50000;

struct BoundSocket {
    int socket_fd;
    const NetworkInterface* interface_ptr;
    bool is_bound;
};

vector<NetworkInterface> network_interfaces;
vector<BoundSocket> bound_sockets;

vector<NetworkInterface> get_network_interfaces() {
    vector<NetworkInterface> interfaces;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return interfaces;
    }

    for( ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || (ifa->ifa_flags & IFF_LOOPBACK)) {
            continue; // Skip null addresses and loopback interfaces
        }

        NetworkInterface interface;
        interface = NetworkInterface::from_ifaddrs(ifa);
        if (interface.ip_address.empty()) {
            continue; // Skip interfaces without an IP address
        }
        interfaces.push_back(interface);
    }
    freeifaddrs(ifaddr);
    return interfaces;
}

int bind_to_interface(const NetworkInterface& interface) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sock);
        return -1;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        close(sock);
        return -1;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        close(sock);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0 , sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    inet_pton(AF_INET, interface.ip_address.c_str(), &addr.sin_addr);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sock);
        return -1;
    }

    cout << "Successfully bound non-blocking socket to: " << interface.name 
         << " (" << interface.ip_address << ":" << DISCOVERY_PORT << ")" << endl;

    return sock;
}

int bind_all_interfaces() {
    vector<NetworkInterface> interfaces = get_network_interfaces();

    for (const auto& interface : interfaces) {
        if (!interface.is_active) {
            cout << "Skipping inactive interface: " << interface.name << endl;
            continue;
        }

        int sockfd = bind_to_interface(interface);
        if (sockfd < 0) {
            cout << "Failed to bind to interface: " << interface.name << endl;
            continue;
        }

        BoundSocket bound_sock;
        bound_sock.socket_fd = sockfd;
        bound_sock.interface_ptr = &interface;
        bound_sock.is_bound = (sockfd >= 0);

        bound_sockets.push_back(bound_sock);
    }
}

void cleanup_sockets() {
    for (auto& bound_sock : bound_sockets) {
        if (bound_sock.is_bound && bound_sock.socket_fd >= 0) {
            close(bound_sock.socket_fd);
            cout << "Closed socket for interface: " << bound_sock.interface_ptr->name << endl;
        }
    }
    bound_sockets.clear();
    cout << "All sockets cleaned up." << endl;
}

int load_network_interfaces() {
    network_interfaces = get_network_interfaces();
    for (const auto& interface : network_interfaces) {
        cout << "Interface: " << interface.name 
             << ", IP Address: " << interface.ip_address 
             << ", Subnet: " << interface.subnet_mask
             << ", Netmask: " << interface.netmask
             << ", MAC Address: " << interface.mac_address
             << ", Active: " << (interface.is_active ? "Yes" : "No") << endl;
    }
    return 0;
}

int main() {
    load_network_interfaces();
    return 0;
}