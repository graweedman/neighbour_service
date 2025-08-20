#ifndef SERVICE_CONNECTION_H
#define SERVICE_CONNECTION_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "common/helper.h"

class ServiceConnection {
    const char* socket_path;

    int connect_to_service() const;
    bool send_command_to_socket(int sock_fd, const std::string& command);
    bool receive_response_from_socket(int sock_fd, std::string& response, size_t buffer_size = 1024);
public:
    ServiceConnection(const char* socket_path);
    ~ServiceConnection();

    bool can_connect() const;
    bool send_and_receive(const std::string& command, std::string& response);
};

#endif // SERVICE_CONNECTION_H