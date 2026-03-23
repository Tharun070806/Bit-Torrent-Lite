#include<stdio.h>
#include<stdlib.h>
#include "socket_utils.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include<arpa/inet.h>
#include <cstring>


void server_thread() {
    int server = create_server_socket(9999);
    std::cout << "[server] listening on 9999\n";
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int client = accept(server, (sockaddr*)&client_addr, &len);
    char buf[64] = {};
    recv_exact(client, buf, 5);
    std::cout << "[server] received: " << buf << "\n";
    send_all(client, buf, 5);
    close(client); close(server);
}

int main() {
    std::thread t(server_thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int fd = connect_to("127.0.0.1", 9999);
    send_all(fd, "hello", 5);
    char buf[6] = {};
    recv_exact(fd, buf, 5);
    std::cout << "[client] echo: " << buf << "\n";
    close(fd);
    t.join();
}