#include "neighbour_discovery.h"
#include <random>

// int NeighbourDiscovery::bind_to_interface(const NetworkInterface& interface) {
//     int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (socket_fd < 0) {
//         std::cerr << "socket failed" << std::endl;
//         return -1;
//     }

//     int reuse = 1;
//     if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
//         std::cerr << "setsockopt SO_REUSEADDR failed" << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
//         std::cerr << "SO_REUSEPORT failed: " << strerror(errno) << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     int enable_broadcast = 1;
//     if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast)) < 0) {
//         std::cerr << "setsockopt SO_BROADCAST failed" << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     // int flags = fcntl(socket_fd, F_GETFL, 0);
//     // if (flags == -1) {
//     //     std::cerr << "fcntl F_GETFL failed" << std::endl;
//     //     close(socket_fd);
//     //     return -1;
//     // }

//     // if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
//     //     std::cerr << "fcntl F_SETFL failed" << std::endl;
//     //     close(socket_fd);
//     //     return -1;
//     // }

//     struct sockaddr_in addr;
//     memset(&addr, 0 , sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(discovery_port);
//     addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
//     if (inet_pton(AF_INET, interface.ip_address.c_str(), &addr.sin_addr) <= 0) {
//         std::cerr << "inet_pton failed for interface IP address" << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         std::cerr << "bind failed" << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     IP_Address broadcast_address = helper::broadcast_address(interface.network_cidr);
//     sockaddr_in broadcast_addr{};
//     broadcast_addr.sin_family = AF_INET;
//     broadcast_addr.sin_port = htons(discovery_port);
//     if (inet_pton(AF_INET, broadcast_address.c_str(), &broadcast_addr.sin_addr) <= 0) {
//         std::cerr << "inet_pton failed for broadcast address" << std::endl;
//         close(socket_fd);
//         return -1;
//     }

//     auto it = std::find_if(interfaces.begin(), interfaces.end(),
//                            [&interface](const NetworkInterface& iface) {
//                                return iface.mac_address == interface.mac_address;
//                            });
//     if (it == interfaces.end()) {
//         std::cerr << "Interface not found in the list: " << interface.name << std::endl;
//         close(socket_fd);
//         return -1;
//     }
//     size_t interface_index = std::distance(interfaces.begin(), it);

//     BoundSocket bound_sock = {socket_fd, interface_index, broadcast_addr, true};
//     bound_sockets.push_back(bound_sock);

//     std::cout << "Successfully bound non-blocking socket to: " << interface.name
//          << " (" << interface.ip_address << ":" << discovery_port << ")" << std::endl;

//     return socket_fd;
// }

int NeighbourDiscovery::bind_all_interfaces() {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    
    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt SO_REUSEADDR failed" << std::endl;
        close(socket_fd);
        return -1;
    }

    int broadcast = 1;
    if(setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "setsockopt SO_BROADCAST failed" << std::endl;
        close(socket_fd);
        return -1;
    }

    int pktinfo = 1;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_PKTINFO, &pktinfo, sizeof(pktinfo)) < 0) {
        std::cerr << "setsockopt IP_PKTINFO failed" << std::endl;
        close(socket_fd);
        return -1;
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        close(socket_fd);
        return -1;
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl F_SETFL failed" << std::endl;
        close(socket_fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(discovery_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed" << std::endl;
        close(socket_fd);
        return -1;
    }

    for (auto& interface : interfaces) {
        // Set up broadcast address for the interface
        std::cout << "Binding to interface: " << interface.name
                  << " (" << interface.ip_address << ":" << discovery_port << ")"
                  << " with broadcast address: " << interface.broadcast_address << std::endl;
        sockaddr_in broadcast_addr{};
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(discovery_port);

        if(inet_pton(AF_INET, interface.broadcast_address.c_str(), &broadcast_addr.sin_addr) <= 0) {
            std::cerr << "inet_pton failed for broadcast address: " << interface.broadcast_address << std::endl;
            close(socket_fd);
            return -1;
        }

        std::cout << "Successfully bound socket to interface: " << interface.name << std::endl;
    }

    return 0;
}

// int NeighbourDiscovery::bind_all_interfaces() {
//     for (const auto& interface : interfaces) {
//         if (interface.is_active) {
//             int socket_fd = bind_to_interface(interface);
//             if (socket_fd < 0) {
//                 std::cerr << "Failed to bind to interface: " << interface.name << std::endl;
//                 continue;
//             }
//         }
//     }
//     if (bound_sockets.empty()) {
//         std::cerr << "No Sockets bound" << std::endl;
//         return -1;
//     }
//     return 0;
// }

void NeighbourDiscovery::cleanup_bound_sockets() {
    close(socket_fd);
}

bool NeighbourDiscovery::neighbor_exists(const NodeIDHex& id) const {
    return neighbors.find(id) != neighbors.end();
}

NetworkNeighbor* NeighbourDiscovery::get_neighbor(const NodeIDHex& id) {
    auto it = neighbors.find(id);
    if (it != neighbors.end()) {
        return &it->second;
    }
    return nullptr;
}

void NeighbourDiscovery::add_or_update_neighbor(const NodeIDHex& id, const NetworkInterface& interface, const NetworkConnection& connection) {
    NetworkNeighbor* neighbor = get_neighbor(id);
    if (neighbor) {
        neighbor->update_last_seen();
        neighbor->add_interface(interface);
        neighbor->add_connection(connection);
    } else {
        NetworkNeighbor new_neighbor;
        new_neighbor.update_last_seen();
        new_neighbor.add_interface(interface);
        new_neighbor.add_connection(connection);
        neighbors[id] = new_neighbor;
    }
}

NeighbourDiscovery::NeighbourDiscovery(const std::vector<NetworkInterface>& interfaces, int discovery_port, NodeID node_id)
    : node_id(node_id), discovery_port(discovery_port), interfaces(interfaces) {
    if (bind_all_interfaces() < 0) {
        std::cerr << "Failed to bind to any interfaces." << std::endl;
    }
}

NeighbourDiscovery::~NeighbourDiscovery() {
    cleanup_bound_sockets();
}

void NeighbourDiscovery::handle_discovery_packet(int socket_fd) {
    char buffer[1024];
    sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    ssize_t bytes_read = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                                   (struct sockaddr*)&sender_addr, &addr_len);
    if (bytes_read < 0) {
        std::cerr << "recvmsg failed" << std::endl;
        return;
    }

    buffer[bytes_read] = '\0'; // Null-terminate the string
    IP_Address sender_ip = inet_ntoa(sender_addr.sin_addr);

    const NetworkInterface* receiving_interface = nullptr;

    for (const auto& interface : interfaces) {
        if (helper::is_ip_in_network(sender_ip, interface.network_cidr)) {
            receiving_interface = &interface;
            break;
        }
    }

    if (!receiving_interface) {
        std::cerr << "No matching interface found for sender IP: " << sender_ip << std::endl;
        return;
    }

    std::string packet_data(buffer);
    listen_for_hello(packet_data, sender_ip, *receiving_interface);
}

void NeighbourDiscovery::handle_activity(const fd_set& read_fds) {
    if (FD_ISSET(socket_fd, &read_fds)) {
        handle_discovery_packet(socket_fd);
    }
}

void NeighbourDiscovery::update()
{
    static auto last_broadcast = std::chrono::steady_clock::now();
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> jitter_dist(0, 1000); // jitter between 0 and 1000 ms

    auto now = std::chrono::steady_clock::now();

    auto broadcast_interval = std::chrono::seconds(5) + std::chrono::milliseconds(jitter_dist(gen));

    if (now - last_broadcast > broadcast_interval) {
        broadcast_hello();
        last_broadcast = now;
    }

    cleanup_inactive_neighbors();
}

void NeighbourDiscovery::cleanup_inactive_neighbors() {
    for (auto it = neighbors.begin(); it != neighbors.end();) {
        if (!it->second.is_active()) {
            std::cout << "Removing inactive neighbor: " << it->first << std::endl;
            it = neighbors.erase(it);
        } else {
            ++it;
        }
    }
}

void NeighbourDiscovery::broadcast_hello() {
    if (socket_fd < 0) {
        std::cerr << "Socket is not valid." << std::endl;
        return;
    }
    
    for (auto& interface : interfaces) {
        NodeIDHex owner_id = node_id_to_hex(node_id);
        NetworkConnection connection;
        connection.ip = interface.ip_address;
        connection.mac_address = interface.mac_address;
        std::string hello_message = "HELLO from " + interface.name +
                                       " NodeID:" + owner_id +
                                       " MAC:" + connection.mac_address +
                                       " IP:" + connection.ip + "\n";
        sockaddr_in broadcast_addr{};
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(discovery_port);
        broadcast_addr.sin_addr.s_addr = interface.broadcast_address.empty() ? INADDR_BROADCAST : inet_addr(interface.broadcast_address.c_str());

        sendto(socket_fd, hello_message.c_str(), hello_message.size(), 0,
            (struct sockaddr*)&broadcast_addr, sizeof(struct sockaddr_in));
        std::cout << "Broadcasted hello on interface: " << interface.name
                  << " (" << interface.ip_address << ":" << discovery_port << ")" << std::endl;
    }
}

void NeighbourDiscovery::listen_for_hello(std::string& hello_message, IP_Address sender_ip, const NetworkInterface& interface) {
    DiscoveryPackage pkg = DiscoveryPackage::from_string(hello_message);
    if (pkg.sender_id.empty() || pkg.sender_ip.empty() || pkg.sender_mac.empty()) {
        std::cerr << "Invalid hello message received." << std::endl;
        return;
    }
    NodeIDHex sender_id = pkg.sender_id;
    NodeIDHex owner_id = node_id_to_hex(node_id);

    if (sender_id == owner_id) {
        return;
    }

    auto it = neighbors.find(sender_id);
    if (it == neighbors.end()) {
        NetworkNeighbor new_neighbor;
        new_neighbor.update_last_seen();
        new_neighbor.add_interface(interface);
        NetworkConnection connection = {sender_ip, pkg.sender_mac};
        new_neighbor.add_connection(connection);
        std::cout << "New neighbor discovered: " << sender_id << std::endl;
        std::cout << "Sender IP: " << pkg.sender_ip << ", MAC: " << pkg.sender_mac << std::endl;
        std::cout << "Interface: " << interface.name << std::endl;
        std::cout << "Network CIDR: " << interface.network_cidr << std::endl;
        neighbors[sender_id] = new_neighbor;
    } else {
        NetworkNeighbor& neighbor = it->second;
        neighbor.update_last_seen();
        NetworkConnection connection = {sender_ip, pkg.sender_mac};
        neighbor.add_interface(interface);
        neighbor.add_connection(connection);
    }
}

const std::unordered_map<NodeIDHex, NetworkNeighbor>& NeighbourDiscovery::get_neighbours() const {
    return neighbors;
}

