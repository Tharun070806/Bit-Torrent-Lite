#include "socket_utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>


int create_server_socket(int port){

    int fd=socket(AF_INET,SOCK_STREAM,0);

    if (fd<0) {
        perror("Socket Initialization Failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //This command allows to reuse the port even if it has just been used and is in the time wait stage

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port        = htons(port);


    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0){
         perror("Socket binding to the port failed ");
         close(fd);
         exit(EXIT_FAILURE);
    }

    if (listen(fd, 10) < 0){
        perror("Listening Failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    cout<<" Socket with port "<<port<<" is listening ......"<<endl;
    
    return fd;

}

int connect_to(const string &host, int port){

    addrinfo addr{}, *pnt;
    addr.ai_family=AF_INET;
    addr.ai_socktype=SOCK_STREAM;

    if(getaddrinfo(host.c_str(), to_string(port).c_str() , &addr, &pnt )!=0){
        cout<<" Error in DNS lookup/ host-ip matching "<<endl;
        return -1;
    }

    int fd= socket(AF_INET, SOCK_STREAM,0);
    
    if(connect(fd, pnt->ai_addr, pnt->ai_addrlen)<0){
        freeaddrinfo(pnt);
        close(fd);
        return -1;
    }

    freeaddrinfo(pnt);
    return fd;

}

bool recv_exact(int fd, void* buf, size_t n){
    size_t received = 0;
    uint8_t* ptr = (uint8_t*)buf;

    while (received < n) {
        // recv() may return LESS than (n - received) bytes 
        // for TCP. We loop until we have all n bytes.
        ssize_t r = recv(fd, ptr + received, n - received, 0);
        if (r <= 0) return false; 
        received += r;
    }
    return true;
}

bool send_all(int fd , const void* buf, size_t n){
    size_t sent = 0;
    const uint8_t* ptr = (const uint8_t*)buf;

    while (sent < n) {
        ssize_t s = send(fd, ptr + sent, n - sent, 0);
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}


