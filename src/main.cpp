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

using namespace std;

const int DISCOVERY_PORT = 50000;
const char* PID_FILE = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
int cli_socket_fd = -1;

struct BoundSocket {
    int socket_fd;
    const NetworkInterface* interface_ptr;
    bool is_bound;
};

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
    cleanup_sockets();

    for (const auto& interface : network_interfaces) {
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
    return 0;
}

void handle_discovery_packet(int socket_fd, const NetworkInterface& interface) {
    char buffer[1024];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    ssize_t bytes_received = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                                      (struct sockaddr*)&sender_addr, &addr_len);
    if (bytes_received < 0) {
        perror("recvfrom failed");
        return;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received data
    cout << "Received packet from " << inet_ntoa(sender_addr.sin_addr) 
         << " on interface " << interface.name << ": " << buffer << endl;

    // Process the packet (e.g., respond to discovery requests)
}

int setup_cli_socket() {
    unlink(CLI_SOCKET_PATH); 

    cli_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cli_socket_fd < 0) {
        perror("CLI socket creation failed");
        return -1;
    }

    int flags = fcntl(cli_socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        close(cli_socket_fd);
        return -1;
    }

    if (fcntl(cli_socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        close(cli_socket_fd);
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CLI_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if(bind(cli_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("CLI socket bind failed");
        close(cli_socket_fd);
        return -1;
    }

    if (listen(cli_socket_fd, 5) < 0) {
        perror("CLI socket listen failed");
        close(cli_socket_fd);
        return -1;
    }

    cout << "CLI socket created and listening at: " << CLI_SOCKET_PATH << endl;
    return 0;
}

void handle_cli_connection() {
    struct sockaddr_un cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    int client_fd = accept(cli_socket_fd, (struct sockaddr*)&cli_addr, &cli_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("CLI accept failed");
        }
        return;
    }

    cout << "CLI client connected" << endl;

    char buffer[256];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        cout << "Received from CLI: " << buffer << endl;
        string command(buffer);

        if (command.find("HELLO") == 0) {
            string response = "WORLD\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
    }

    close(client_fd);
    cout << "CLI client disconnected" << endl;
}

void cleanup_cli_socket() {
    if (cli_socket_fd >= 0) {
        close(cli_socket_fd);
        unlink(CLI_SOCKET_PATH);
        cli_socket_fd = -1;
        cout << "CLI socket cleaned up." << endl;
    }
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
    write_pid_file();
    cout << "Starting network discovery service..." << endl;
    if (load_network_interfaces() < 0) {
        cerr << "Failed to load network interfaces." << endl;
        return 1;
    }
    if (setup_cli_socket() < 0) {
        cerr << "Failed to set up CLI socket." << endl;
        return 1;
    }

    cout << "Binding to all active interfaces..." << endl;
    if (bind_all_interfaces() < 0) {
        cerr << "Failed to bind to all active interfaces." << endl;
        return 1;
    }

    if (bound_sockets.empty()) {
        cout << "No active interfaces found to bind." << endl;
        return 1;
    } else {
        cout << "Successfully bound to " << bound_sockets.size() << " active interfaces." << endl;
        for (const auto& bound_sock : bound_sockets) {
            cout << " - " << bound_sock.interface_ptr->name << " (" << bound_sock.interface_ptr->ip_address << ")" << endl;
        }
    }

    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = cli_socket_fd;

        for (const auto& bound_sock : bound_sockets) {
            if (bound_sock.is_bound) {
                FD_SET(bound_sock.socket_fd, &read_fds);
                if (bound_sock.socket_fd > max_fd) {
                    max_fd = bound_sock.socket_fd;
                }
            }
        }

        FD_SET(cli_socket_fd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 5; // Wait for 5 seconds
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            perror("select failed");
            break;
        } else if (activity == 0) {
            cout << "No activity detected, continuing..." << endl;
            continue;
        }

        if (FD_ISSET(cli_socket_fd, &read_fds)) {
            handle_cli_connection();
        }

        for (auto& bound_sock : bound_sockets) {
            if (FD_ISSET(bound_sock.socket_fd, &read_fds)) {
                handle_discovery_packet(bound_sock.socket_fd, *bound_sock.interface_ptr);
            }
        }
    }       

    cleanup_sockets();
    cleanup_cli_socket();
    cleanup_pid_file();
    cout << "Network discovery service finished." << endl;
    cout << "Exiting..." << endl;

    return 0;
}