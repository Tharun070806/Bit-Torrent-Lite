#pragma once
#include<string>
#include<cstdint>
using namespace std;

int create_server_socket(int port);

int connect_to(const string &host, int port,int client_port);

bool recv_exact(int fd, void* buf, size_t n);

bool send_all(int fd, const void* buf, size_t n);

void recv_inf(std::string &response, char*buffer,int socket);

void close_socket(int socket);