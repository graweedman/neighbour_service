#ifndef MAIN_H
#define MAIN_H

#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <vector>

#include "common/types.h"
#include "common/helper.h"
#include "common/node_id.h"
#include "service.h"

using namespace std;

const int DISCOVERY_PORT = 50000;
const char* PID_FILE = "/tmp/graw_service.pid";
const char* CLI_SOCKET_PATH = "/tmp/graw_service.sock";
int cli_socket_fd = -1;
bool quiet_mode = true;
bool running = true;

vector<NetworkInterface> network_interfaces;

void write_pid_file();
void cleanup_pid_file();
vector<NetworkInterface> get_network_interfaces();
int main();

#endif // MAIN_H