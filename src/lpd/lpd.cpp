#include "lpd.hpp"
#include "../utils/socket_utils.hpp"
#include "../peer/peer.hpp"
#include <iostream>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <unordered_set>

void lpd_broadcast_thread(std::string info_hash_hex, int tcp_port, std::string my_cookie) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sock < 0) return;

   
    int ttl = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("239.192.152.143");
    addr.sin_port = htons(6771);

    std::string message = 
        "BT-SEARCH * HTTP/1.1\r\n"
        "Host: 239.192.152.143:6771\r\n"
        "Port: " + std::to_string(tcp_port) + "\r\n"
        "Infohash: " + info_hash_hex + "\r\n"
        "cookie: " + my_cookie + "\r\n\r\n";

    std::cout << "[LPD] Broadcaster thread started.\n";

    while(true) {
        sendto(sock, message.c_str(), message.length(), 0, (struct sockaddr*)&addr, sizeof(addr));
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

void lpd_listen_thread(std::string my_info_hash, std::string my_cookie, const handshake& h, 
                       Controller& manager, std::vector<std::thread>& all_threads, std::mutex& mtx) {
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

   
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(6771);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "[LPD] Failed to bind to 6771.\n";
        close(sock);
        return;
    }

    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr("239.192.152.143");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        std::cerr << "[LPD] Failed to join multicast group 239.192.152.143.\n";
        close(sock);
        return;
    }

    std::cout << "[LPD] Listener thread actively scanning for LAN peers.\n";
    char buffer[2048];
    std::unordered_set<std::string> known_peers; 

    while(true) {
        sockaddr_in sender_addr{};
        socklen_t addr_len = sizeof(sender_addr);
        
        int bytes = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&sender_addr, &addr_len);
        if (bytes <= 0) continue;
        
        buffer[bytes] = '\0';
        std::string payload(buffer);

   
        size_t infohash_pos = payload.find("Infohash: ");
        size_t port_pos = payload.find("Port: ");
        size_t cookie_pos = payload.find("cookie: ");

        if (infohash_pos != std::string::npos && port_pos != std::string::npos) {
            
            std::string extracted_hash = payload.substr(infohash_pos + 10, 40); 
            
            size_t port_end = payload.find("\r", port_pos);
            int peer_tcp_port = std::stoi(payload.substr(port_pos + 6, port_end - (port_pos + 6)));
            
            std::string extracted_cookie = "";
            if (cookie_pos != std::string::npos) {
                size_t cookie_end = payload.find("\r", cookie_pos);
                extracted_cookie = payload.substr(cookie_pos + 8, cookie_end - (cookie_pos + 8));
            }

            if (extracted_cookie == my_cookie) {
                continue; 
            }

            if (extracted_hash == my_info_hash) {
                std::string peer_ip = inet_ntoa(sender_addr.sin_addr);
                std::string identifier = peer_ip + ":" + std::to_string(peer_tcp_port);

           
                if (known_peers.find(identifier) != known_peers.end()) {
                    continue; 
                }
                
                known_peers.insert(identifier);
                std::cout << "[LPD] Discovery match! Bootstrapping local peer: " << identifier << "\n";
                
                int tcp_fd = connect_to(peer_ip, peer_tcp_port);
                if (tcp_fd >= 0) {
                    std::lock_guard<std::mutex> lock(mtx);
                    all_threads.emplace_back(peer_worker, tcp_fd, std::ref(h), std::ref(manager), peer_ip, peer_tcp_port);
                } else {
                    std::cerr << "[LPD] Could not establish connection to local peer.\n";
                    known_peers.erase(identifier); // Free the lock if connection failed so we can retry later
                }
            }
        }
    }
}
