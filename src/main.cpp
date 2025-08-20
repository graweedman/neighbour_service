#include "main.h"

using namespace std;

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
        helper::log_error("getifaddrs", quiet_mode);
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

int main() {
    write_pid_file();
    Service service(CLI_SOCKET_PATH, DISCOVERY_PORT, quiet_mode);

    if (service.start() != 0) {
        helper::log_error("Failed to start service.", quiet_mode);
        return -1;
    }
    service.loop();

    service.stop();
    cleanup_pid_file();
    helper::log_info("Network discovery service finished.", quiet_mode);
    helper::log_info("Exiting...", quiet_mode);

    return 0;
}