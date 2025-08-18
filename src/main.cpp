#include "main.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/select.h>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <string>

#include "common/types.h"
#include "common/helper.h"
#include "common/node_id.h"
#include "service.h"

using namespace std;

const int DISCOVERY_PORT = 50000;
const char* PID_FILE = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
int cli_socket_fd = -1;

bool running = true;

vector<NetworkInterface> network_interfaces;
vector<BoundSocket> bound_sockets;


void write_pid_file() {
    FILE* pid_file = fopen(PID_FILE, "w");
    if (pid_file) {
        fprintf(pid_file, "%d\n", getpid());
        fclose(pid_file);
    }
}

void cleanup_pid_file() {
    unlink(PID_FILE);
}

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

int load_network_interfaces() {
    network_interfaces = get_network_interfaces();
    for (const auto& interface : network_interfaces) {
        cout << "Interface: " << interface.name 
             << ", IP Address: " << interface.ip_address 
             << ", Subnet: " << interface.subnet_mask
             << ", MAC Address: " << interface.mac_address
             << ", Active: " << (interface.is_active ? "Yes" : "No") << endl;
    }
    return 0;
}

int main() {
    write_pid_file();
    Service service(CLI_SOCKET_PATH, DISCOVERY_PORT);

    if (service.start() != 0) {
        cerr << "Failed to start service." << endl;
        return -1;
    }

    while (running) {
        service.loop();
    }

    service.stop();
    cleanup_pid_file();
    cout << "Network discovery service finished." << endl;
    cout << "Exiting..." << endl;

    return 0;
}