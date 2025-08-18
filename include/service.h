#ifndef SERVICE_H
#define SERVICE_H

#include <ifaddrs.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <unistd.h>
#include <memory>

#include "neighbour_discovery.h"
#include "common/types.h"
#include "common/node_id.h"

class Service {
    NodeID node_id;
    const char* cli_socket_path;
    int cli_socket_fd;
    int discovery_port;

    std::unique_ptr<NeighbourDiscovery> neighbour_discovery;
    std::vector<NetworkInterface> interfaces;

    int init();
    int update_network_interfaces();
    int init_interfaces();
    int init_cli_socket();
    void cleanup_cli_socket();
    void handle_cli_connection();
public:
    Service(const char* cli_socket_path, int discovery_port);
    ~Service();

    int start();
    int loop();
    void stop();
    void update_neighbors();
    std::vector<NetworkNeighbor> get_neighbors() const;
    std::vector<NetworkInterface> get_interfaces() const;
};

#endif // SERVICE_H