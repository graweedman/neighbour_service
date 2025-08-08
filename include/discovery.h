#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "common/types.h"
#include <vector>

class Discovery {
    std::vector<NetworkNeighbor> neighbours;
public:
    Discovery();
    ~Discovery();

    void update();
    void broadcast_hello();
    void listen_for_hello();
    std::vector<NetworkNeighbor> get_neighbours() const;
};

#endif // DISCOVERY_H