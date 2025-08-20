#include "common/node_id.h"
#include <iostream>

static const char* kNodeIdDir  = ".local/share/neighsvc";
static const char* kNodeIdFile = ".local/share/neighsvc/node_id";

static bool read_file(const char* path, uint8_t* buf, size_t len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;
    ssize_t r = read(fd, buf, len);
    close(fd);
    return r == (ssize_t)len;
}

static bool write_file(const char* path, const uint8_t* buf, size_t len) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return false;
    ssize_t w = write(fd, buf, len);
    fsync(fd);
    close(fd);
    return w == (ssize_t)len;
}

static void set_uuid_v4_bits(uint8_t* b16) {
    b16[6] = (uint8_t)((b16[6] & 0x0F) | 0x40); // version 4
    b16[8] = (uint8_t)((b16[8] & 0x3F) | 0x80); // variant 10
}

static bool gen_random_16(uint8_t* out) {
    memset(out, 0, 16);

    ssize_t r = getrandom(out, 16, GRND_NONBLOCK);
    if (r == 16) return true;

    memset(out, 0, 16);

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return false;

    ssize_t rr = read(fd, out, 16);
    close(fd);

    return rr == 16;
}

static bool ensure_dir(const char* dirpath) {
    struct stat st{};
    if (stat(dirpath, &st) == 0) return S_ISDIR(st.st_mode);
    std::string path_str(dirpath);
    size_t pos = 0;
    while ((pos = path_str.find('/', pos + 1)) != std::string::npos) {
        std::string subdir = path_str.substr(0, pos);
        if (mkdir(subdir.c_str(), 0700) != 0 && errno != EEXIST) {
            return false;
        }
    }

    return !(mkdir(dirpath, 0700) != 0 && errno != EEXIST);
}

NodeID generate_node_id()
{
    NodeID id{};
    const char* home = getenv("HOME");
    if (!home) return id;

    char dirpath[1024];
    char filepath[1024];
    snprintf(dirpath, sizeof(dirpath), "%s/%s", home, kNodeIdDir);
    snprintf(filepath, sizeof(filepath), "%s/%s", home, kNodeIdFile);

    if (read_file(filepath, id.data(), id.size())) return id;

    if (!ensure_dir(dirpath)) return id;

    uint8_t tmp[16];
    if (!gen_random_16(tmp)) {
        return id;
    }
    set_uuid_v4_bits(tmp);

    if (!write_file(filepath, tmp, 16)) return id;

    std::memcpy(id.data(), tmp, 16);
    return id;
}

NodeIDHex node_id_to_hex(const NodeID& id) {
    char hex_str[33]; // 32 hex digits + null terminator
    for (size_t i = 0; i < id.size(); ++i) {
        snprintf(hex_str + i * 2, 3, "%02x", id[i]);
    }
    return NodeIDHex(hex_str);
}