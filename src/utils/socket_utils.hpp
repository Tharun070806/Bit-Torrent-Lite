#pragma once
#include<string>
#include<cstdint>
using namespace std;

int create_server_socket(int port);

int connect_to(const string &host, int port);

bool recv_exact(int fd, void* buf, size_t n);

bool send_all(int fd, const void* buf, size_t n);
std::string recv_exact(int socket_fd, size_t size);
bool send_all(int fd , std::string &message, size_t n);
std::string recv_inf(int socket_fd);
void write_all(int socket_fd, const std::string& data);
void recv_inf(std::string &response, char*buffer,int socket);
bool send_all(int fd, std::vector<uint8_t>arr);
void close_socket(int socket);