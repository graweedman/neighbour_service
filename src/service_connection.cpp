#include "service_connection.h"

ServiceConnection::ServiceConnection(const char* socket_path)
    : socket_path(socket_path) {}

ServiceConnection::~ServiceConnection() {
}

int ServiceConnection::connect_to_service() const {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection to service failed: " << strerror(errno) << std::endl;
        close(sock_fd);
        return -1;
    }
    return sock_fd;
}

bool ServiceConnection::send_command_to_socket(int sock_fd, const std::string &command)
{
    std::string formatted_command = command + "\n";
    ssize_t bytes_sent = send(sock_fd, formatted_command.c_str(), formatted_command.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send command: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool ServiceConnection::receive_response_from_socket(int sock_fd, std::string &response, size_t buffer_size)
{
    char buffer[buffer_size];
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 5; // Wait for 5 seconds
    timeout.tv_usec = 0;
    int activity = select(sock_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (activity < 0) {
        std::cerr << "select failed" << std::endl;
        return false;
    } else if (activity == 0) {
        std::cerr << "Timeout waiting for response" << std::endl;
        return false;
    }

    ssize_t bytes_received = recv(sock_fd, buffer, buffer_size - 1, 0);
    if (bytes_received < 0) {
        std::cerr << "Failed to receive response: " << strerror(errno) << std::endl;
        return false;
    }
    
    buffer[bytes_received] = '\0';
    response = std::string(buffer);
    return true;
}

bool ServiceConnection::send_and_receive(const std::string& command, std::string& response) {
    int sock_fd = connect_to_service();
    if (sock_fd < 0) {
        std::cerr << "Failed to connect to service" << std::endl;
        return false;
    }

    bool success = send_command_to_socket(sock_fd, command);
    if (!success) {
        std::cerr << "Failed to send command to service" << std::endl;
        close(sock_fd);
        return false;
    }

    success = receive_response_from_socket(sock_fd, response);
    close(sock_fd);
    return success;
}

bool ServiceConnection::can_connect() const {
    int sock_fd = connect_to_service();
    if (sock_fd < 0) {
        std::cerr << "Cannot connect to service" << std::endl;
        return false;
    }
    close(sock_fd);
    return true;
}
