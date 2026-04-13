#include "socket_utils.hpp"



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

int connect_to_specific(const string &host, int port, int local_port){

    addrinfo addr{}, *pnt;
    addr.ai_family=AF_INET;
    addr.ai_socktype=SOCK_STREAM;

    if(getaddrinfo(host.c_str(), to_string(port).c_str() , &addr, &pnt )!=0){
        cout<<" Error in DNS lookup/ host-ip matching "<<endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;  // any local IP
        local_addr.sin_port = htons(local_port);

        // allow reuse (important if reconnecting)
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(fd, (sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            perror("bind failed");
            close(fd);
            return -1;
        }

    
    
    if(connect(fd, pnt->ai_addr, pnt->ai_addrlen)<0){
        freeaddrinfo(pnt);
        close(fd);
        return -1;
    }

    freeaddrinfo(pnt);
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

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    
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

std::string recv_exact(int socket_fd, size_t size) {
    std::string data(size, '\0');
    size_t read_offset = 0;
    while (read_offset < size) {
        ssize_t bytes_read = recv(socket_fd, data.data() + read_offset, size - read_offset, 0);
        if (bytes_read == 0) {
            throw std::runtime_error("Connection closed before receiving expected data");
        }
        if (bytes_read < 0) {
            throw std::runtime_error("Failed to read peer response: " + std::string(std::strerror(errno)));
        }
        read_offset += static_cast<size_t>(bytes_read);
    }
    return data;
}
void write_all(int socket_fd, const std::string& data) {
    size_t written = 0;
    while (written < data.size()) {
        ssize_t bytes_written = send(socket_fd, data.data() + written, data.size() - written, 0);
        if (bytes_written <= 0) {
            throw std::runtime_error("Failed to write to socket: " + std::string(std::strerror(errno)));
        }
        written += static_cast<size_t>(bytes_written);
    }
}

bool send_all(int fd, std::vector<uint8_t>arr){
    int n=arr.size();
    return send_all(fd,arr.data(),n);
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

bool send_all(int fd , std::string &message, size_t n){
    size_t sent = 0;
    const char* ptr = message.data();

    while (sent < n) {
        ssize_t s = send(fd, ptr + sent, n - sent, 0);
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}

//When we dont know what is the length of the message incoming use recv_info

void recv_inf(std::string &response, char*buffer,int socket){
        
        int bytes;

        while ((bytes = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
             response.append(buffer, bytes);
        }
    }

    std::string recv_inf(int socket_fd) {
    std::string data;
    std::array<char, 4096> buffer{};
    while (true) {
        ssize_t bytes_read = recv(socket_fd, buffer.data(), buffer.size(), 0);
        if (bytes_read == 0) {
            return data;
        }
        if (bytes_read < 0) {
            throw std::runtime_error("Failed to read tracker response: " + std::string(std::strerror(errno)));
        }
        data.append(buffer.data(), static_cast<size_t>(bytes_read));
    }
}

void close_socket(int socket){
    close(socket);
}


