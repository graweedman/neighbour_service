#include "cli.h"

using namespace std;

ServiceConnection service_connection(CLI_SOCKET_PATH);


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
    helper::log_info("Starting service...", false);

    pid_t pid = fork();
    if (pid < 0) {
        helper::log_error("fork failed", false);
        return -1;
    } else if (pid == 0) {
        execl(SERVICE_BINARY, "graw_service", nullptr);
        helper::log_error("execl failed", false);
        exit(EXIT_FAILURE);
    }

    helper::log_info("Service started with PID: " + std::to_string(pid), false);
    sleep(2);

    for( int i = 0; i < 5; ++i) {
        if (is_service_running()) {
            helper::log_info("Service is ready.", false);
            return 0;
        }
        helper::log_info("Waiting for service to be ready...", false);
        usleep(500000);
    }

    helper::log_error("Service did not start in time.", false);
    return -1;
}

void stop_service() {
    pid_t pid = service_pid;

    if (pid <= 0) {
        pid = read_pid_file();
    }

    if (pid > 0) {
        helper::log_info("Stopping service (PID: " + std::to_string(pid) + ")", false);
        if (kill(pid, SIGTERM) == 0) {
            int status;
            if (waitpid(pid, &status, WNOHANG) >= 0) {
                helper::log_info("Service stopped", false);
            }
            service_pid = -1;
            unlink(SERVICE_PID_PATH);
        }
    } else {
        helper::log_info("No running service found.", false);
    }
}

int main() {
    helper::log_info("Connecting to neighbor discovery service...", false);

    while (true) {
        cout << "> ";
        string input;
        getline(cin, input);

        if (input == "quit" || input == "exit") {
            break;
        }

        if (input == "help") {
            cout << "Available commands:" << endl;
            cout << "start - Start the neighbor discovery service" << endl;
            cout << "stop - Stop the neighbor discovery service" << endl;
            cout << "status - Check the status of the neighbor discovery service" << endl;
            cout << "help - Show this help message" << endl;
            cout << "LIST - List all discovered neighbors from service" << endl;
            cout << "PING - Send a PING request to the service" << endl;
            cout << "quit - Exit the CLI" << endl;
            continue;
        }

        if (input == "start") {
            if (is_service_running() && service_connection.can_connect()) {
                helper::log_info("Service is already running", false);
            } else {
                start_service();
            }
            continue;
        }

        if (input == "stop") {
            helper::log_info("Stopping service...", false);
            stop_service();
            continue;
        }

        if (input == "status") {
            if (is_service_running()) {
                helper::log_info("Service is running", false);
            } else {
                helper::log_info("Service is not running", false);
            }
            if (service_connection.can_connect()) {
                helper::log_info("Can connect to service.", false);
            } else {
                helper::log_info("Cannot connect to service.", false);
            }
            continue;
        }

        if (input.empty()) {
            continue;
        }
        string response;
        if (service_connection.send_and_receive(input, response)) {
            cout << "Response: " + response << endl;
        } else {
            helper::log_error("Failed to send command or receive response.", false);
        }
    }
    stop_service();
    helper::log_info("Disconnected from service.", false);

    return 0;
}