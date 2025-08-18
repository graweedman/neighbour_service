#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <unordered_map>
#include <ifaddrs.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <unistd.h>

#include "common/types.h"
#include "common/helper.h"
#include "common/node_id.h"

class NeighbourDiscovery {
    NodeID node_id;
    int discovery_port;
    const std::vector<NetworkInterface>& interfaces;
    std::unordered_map<NodeIDHex, NetworkNeighbor> neighbors;
    std::vector<BoundSocket> bound_sockets;

    void handle_discovery_packet(int socket_fd, const NetworkInterface& interface);

    int bind_to_interface(const NetworkInterface& interface);
    int bind_all_interfaces();
    void cleanup_bound_sockets();

    bool neighbor_exists(const NodeIDHex& id) const;
    NetworkNeighbor* get_neighbor(const NodeIDHex& id);
    void add_or_update_neighbor(const NodeIDHex& id, const NetworkInterface& interface, const NetworkConnection& connection);
public:
    NeighbourDiscovery(const std::vector<NetworkInterface>& interfaces, int discovery_port, NodeID node_id);
    ~NeighbourDiscovery();
    void handle_activity(const fd_set& read_fds);
    void update();
    void cleanup_inactive_neighbors();
    void broadcast_hello();
    void listen_for_hello(std::string& hello_message, const NetworkInterface& interface);
    const std::unordered_map<NodeIDHex, NetworkNeighbor>& get_neighbours() const;
    const std::vector<BoundSocket>& get_bound_sockets() const;
};

#endif // DISCOVERY_H