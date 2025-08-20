#ifndef CLI_H
#define CLI_H

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>

#include "common/helper.h"
#include "service_connection.h"

using namespace std;

const char* SERVICE_PID_PATH = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
const char* SERVICE_BINARY = "./graw_service";
pid_t service_pid = -1;

pid_t read_pid_file();
bool is_service_running();
int start_service();
void stop_service();
int main();

#endif // CLI_H
