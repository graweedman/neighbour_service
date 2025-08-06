#ifndef SERVICE_H
#define SERVICE_H

#include <ifaddrs.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "common/types.h"

class Service {
    std::vector<NetworkInterface> interfaces;
    std::unordered_map<std::string, NetworkNeighbor> neighbors;
public:
    Service();

    ~Service() = default;
};

#endif // SERVICE_H