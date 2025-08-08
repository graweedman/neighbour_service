#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>


using namespace std;

const char* SERVICE_PID_PATH = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
const char* SERVICE_BINARY = "./build/graw_service";


pid_t service_pid = -1;

pid_t read_pid_file() {
    FILE* pid_file = fopen(SERVICE_PID_PATH, "r");
    if (!pid_file) {
        return -1;
    }

    pid_t pid = -1;
    fscanf(pid_file, "%d", &pid);
    fclose(pid_file);

    if (kill(pid, 0) == 0) {
        return pid;
    }

    unlink(SERVICE_PID_PATH);
    return -1;
}

bool socket_exists() {
    struct stat buffer;
    return (stat(CLI_SOCKET_PATH, &buffer) == 0);
}

bool can_connect_to_service() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket creation failed");
        return false;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, CLI_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    bool can_connect = (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0);
    close(sock_fd);
    return can_connect;
}

int start_service() {
    cout << "Starting service..." << endl;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        execl("./build/graw_service", "graw_service", nullptr);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }

    cout << "Service started with PID: " << pid << endl;
    sleep(2);

    for( int i = 0; i < 5; ++i) {
        if (can_connect_to_service()) {
            cout << "Service is ready." << endl;
            return 0;
        }
        cout << "Waiting for service to be ready..." << endl;
        usleep(500000);
    }

    cout << "Service did not start in time." << endl;
    return -1;
}

void stop_service() {
    pid_t pid = service_pid;

    if (pid <= 0) {
        pid = read_pid_file();
    }

    if (pid > 0) {
        cout << "Stopping service (POD: " << pid << ")..." << endl;
        if (kill(pid, SIGTERM) == 0) {
            int status;
            if (waitpid(pid, &status, WNOHANG) >= 0) {
                cout << "Service stopped" << endl;
            }
            service_pid = -1;
            unlink(SERVICE_PID_PATH);
        }
    } else {
        cout << "No running service found." << endl;
    }
}

int connect_to_service() {
    if (!socket_exists() || !can_connect_to_service()) {
        cout << "Service is not running, starting it..." << endl;
        if (start_service() < 0) {
            cerr << "Failed to start service." << endl;
            return -1;
        }
    } else {
        cout << "Service is already running." << endl;
    }

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, CLI_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed - is the service running?");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
    
}

ssize_t recv_with_timeout(int sock_fd, char* buffer, size_t buffer_size, int timeout_seconds) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);
    
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    int result = select(sock_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (result < 0) {
        perror("select failed");
        return -1;
    } else if (result == 0) {
        cout << "Timeout waiting for response" << endl;
        return 0;  // Timeout
    }
    
    return recv(sock_fd, buffer, buffer_size, 0);
}

bool is_service_running() {
    return (read_pid_file() > 0) || can_connect_to_service();
}

int main() {
    cout << "Connecting to neighbor discovery service..." << endl;

    while (true) {
        cout << "> ";
        string input;
        getline(cin, input);

        if (input == "quit" || input == "exit") {
            break;
        }

        if (input == "start") {
            if (is_service_running()) {
                cout << "Service is already running" << endl;
            } else {
                start_service();
            }
            continue;
        }

        if (input == "stop") {
            stop_service();
            continue;
        }

        if (input == "status") {
            if (is_service_running()) {
                cout << "Service is running" << endl;
            } else {
                cout << "Service is not running" << endl;
            }
            continue;
        }

        // Regular commands - auto-start service if needed
        int sock_fd = connect_to_service();
        if (sock_fd < 0) {
            cout << "Failed to connect to service" << endl;
            continue;
        }

        // Send command
        string command = input + "\n";
        if (send(sock_fd, command.c_str(), command.length(), 0) < 0) {
            perror("send failed");
            close(sock_fd);
            continue;
        }

        // Receive response
        char buffer[1024];
        ssize_t bytes_received = recv_with_timeout(sock_fd, buffer, sizeof(buffer) -1, 5);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "Response: " << buffer;
        } else {
            cout << "No response from service" << endl;
        }

        close(sock_fd);
    }

    stop_service();
    cout << "Disconnected from service." << endl;

    return 0;
}