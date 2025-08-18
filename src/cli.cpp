#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>

#include "service_connection.h"


using namespace std;

const char* SERVICE_PID_PATH = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
const char* SERVICE_BINARY = "./graw_service";

ServiceConnection service_connection(CLI_SOCKET_PATH);

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

bool is_service_running() {
    return (read_pid_file() > 0);
}

int start_service() {
    cout << "Starting service..." << endl;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        execl(SERVICE_BINARY, "graw_service", nullptr);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }

    cout << "Service started with PID: " << pid << endl;
    sleep(2);

    for( int i = 0; i < 5; ++i) {
        if (is_service_running()) {
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
            if (is_service_running() && service_connection.can_connect()) {
                cout << "Service is already running" << endl;
            } else {
                start_service();
            }
            continue;
        }

        if (input == "stop") {
            cout << "Stopping service..." << endl;
            stop_service();
            continue;
        }

        if (input == "status") {
            if (is_service_running()) {
                cout << "Service is running" << endl;
            } else {
                cout << "Service is not running" << endl;
            }
            if (service_connection.can_connect()) {
                cout << "Can connect to service." << endl;
            } else {
                cout << "Can not connect to service." << endl;
            }
            continue;
        }

        if (input.empty()) {
            continue;
        }
        string response;
        if (service_connection.send_and_receive(input, response)) {
            cout << "Response: " << response << endl;
        } else {
            cout << "Failed to send command or receive response." << endl;
        }
    }
    stop_service();
    cout << "Disconnected from service." << endl;

    return 0;
}