#include "service.h"

Service::Service(const char* cli_socket_path, int discovery_port, bool quiet_mode)
    : cli_socket_path(cli_socket_path), cli_socket_fd(-1), discovery_port(discovery_port), quiet_mode(quiet_mode)
{
    node_id = generate_node_id();
}

Service::~Service() {
    if (cli_socket_fd >= 0) {
        cleanup_cli_socket();
    }
}

int Service::init() {
    if (init_interfaces() < 0) {
        helper::log_error("Failed to initialize network interfaces.", quiet_mode);
        return -1;
    }
    if (init_cli_socket() < 0) {
        helper::log_error("Failed to initialize CLI socket.", quiet_mode);
        return -1;
    }

    neighbour_discovery = std::make_unique<NeighbourDiscovery>(interfaces, discovery_port, node_id, quiet_mode);
    if (!neighbour_discovery) {
        helper::log_error("Failed to create NeighbourDiscovery instance.", quiet_mode);
        return -1;
    } else {
        helper::log_info("NeighbourDiscovery initialized with discovery port: " + std::to_string(discovery_port), quiet_mode);
        neighbour_discovery->broadcast_hello();
    }

    return 0;
}


int Service::update_network_interfaces()
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        helper::log_error("getifaddrs failed", quiet_mode);
        return -1;
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
        std::cout << "Interface: " << interface.name 
                  << ", IP: " << interface.ip_address 
                  << ", MAC: " << interface.mac_address 
                  << ", Subnet Mask: " << interface.subnet_mask 
                  << ", Network CIDR: " << interface.network_cidr 
                  << ", Broadcast Address: " << interface.broadcast_address 
                  << std::endl;
        interfaces.push_back(interface);
    }
    freeifaddrs(ifaddr);
    return 0;
}

int Service::init_interfaces()
{
    if (update_network_interfaces() < 0) {
        helper::log_error("Failed to update network interfaces.", quiet_mode);
        return -1;
    }
    if (interfaces.empty()) {
        helper::log_error("No active network interfaces found.", quiet_mode);
        return -1;
    }
    return 0;
}

int Service::init_cli_socket()
{
    unlink(cli_socket_path); 

    cli_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cli_socket_fd < 0) {
        helper::log_error("CLI socket creation failed", quiet_mode);
        return -1;
    }

    int flags = fcntl(cli_socket_fd, F_GETFL, 0);
    if (flags == -1) {
        helper::log_error("fcntl F_GETFL failed", quiet_mode);
        close(cli_socket_fd);
        return -1;
    }

    if (fcntl(cli_socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        helper::log_error("fcntl F_SETFL failed", quiet_mode);
        close(cli_socket_fd);
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, cli_socket_path, sizeof(addr.sun_path) - 1);

    if(bind(cli_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        helper::log_error("CLI socket bind failed", quiet_mode);
        close(cli_socket_fd);
        return -1;
    }

    if (listen(cli_socket_fd, 5) < 0) {
        helper::log_error("CLI socket listen failed", quiet_mode);
        close(cli_socket_fd);
        return -1;
    }

    helper::log_info("CLI socket created and listening at: " + std::string(cli_socket_path), quiet_mode);
    return 0;
}

void Service::cleanup_cli_socket() {
    if (cli_socket_fd >= 0) {
        close(cli_socket_fd);
        unlink(cli_socket_path);
        cli_socket_fd = -1;
        helper::log_info("CLI socket cleaned up.", quiet_mode);
    }
}

void Service::handle_cli_connection() {
    struct sockaddr_un cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    int client_fd = accept(cli_socket_fd, (struct sockaddr*)&cli_addr, &cli_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("CLI accept failed");
        }
        return;
    }

    helper::log_info("CLI client connected", quiet_mode);

    char buffer[256];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        helper::log_info("Received from CLI: " + std::string(buffer), quiet_mode);
        std::string command(buffer);

        if (command.find("PING") == 0) {
            std::string response = "WORLD\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        if (command.find("LIST") == 0) {
            const auto& neighbors = neighbour_discovery->get_neighbours();
            std::string response = "Neighbors:\n";

            if (neighbors.empty()) {
                response = "No neighbors found.\n";
            } else {
                for (const auto& [id, neighbor] : neighbors) {
                    std::string connections = "Connections:\n";
                    std::string interfaces = "Interfaces:\n";
                    for (const auto& conn : neighbor.connections) {
                        connections += " - " + conn.ip + " (" + conn.mac_address + ")\n";
                    }
                    for (const auto& iface_name : neighbor.interface_names) {
                        interfaces += " - " + iface_name + "\n";
                    }
                    response += id + " - " + connections + interfaces + "\n";
                }
            }
            
            send(client_fd, response.c_str(), response.size(), 0);
        }
    }

    close(client_fd);
    helper::log_info("CLI client disconnected", quiet_mode);
}

int Service::start()
{
    if (init() < 0) {
        std::cerr << "Service initialization failed." << std::endl;
        return -1;
    }
    return 0;
}

int Service::loop()
{
    running = true;

    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = 0;

        if (cli_socket_fd >= 0) {
            FD_SET(cli_socket_fd, &read_fds);
            max_fd = cli_socket_fd;
        }

        if (neighbour_discovery) {
            FD_SET(neighbour_discovery->get_socket_fd(), &read_fds);
            max_fd = std::max(max_fd, neighbour_discovery->get_socket_fd());
        }

        struct timeval timeout;
        timeout.tv_sec = 5; // Wait for 5 seconds
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0) {
            helper::log_error("select failed", quiet_mode);
            return -1;
        }

        if (activity > 0 && cli_socket_fd >= 0 && FD_ISSET(cli_socket_fd, &read_fds))
        {
            handle_cli_connection();
        }
        if (activity > 0 && neighbour_discovery) {
            neighbour_discovery->handle_activity(read_fds);
        }

        if (neighbour_discovery) {
            neighbour_discovery->update();
        }
    }

    return 0;
}

void Service::stop()
{
    if (neighbour_discovery) {
        neighbour_discovery->cleanup_inactive_neighbors();
    }
    cleanup_cli_socket();
}

std::vector<NetworkInterface> Service::get_interfaces() const {
    return interfaces;
}
