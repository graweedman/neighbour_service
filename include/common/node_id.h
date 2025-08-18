#ifndef COMMON_NODE_ID_H
#define COMMON_NODE_ID_H

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/random.h>
#include <fcntl.h>

using NodeID = std::array<uint8_t, 16>; // 16 bytes for Node ID
typedef std::string NodeIDHex;

NodeID generate_node_id();
NodeIDHex node_id_to_hex(const NodeID& id);

#endif // COMMON_NODE_ID_H