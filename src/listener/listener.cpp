// Listener.hpp

#include "listener.hpp"

void listener_thread(int listen_fd, const handshake& h, Controller& manager,
                     std::vector<std::thread>& all_threads,
                     std::mutex& threads_mutex) {

    std::cout << "[listener] ready\n";

    while (true) {
        sockaddr_in peer_addr{};
        socklen_t   len = sizeof(peer_addr);
        int peer_fd = accept(listen_fd, (sockaddr*)&peer_addr, &len);
        if (peer_fd < 0) break; // listen_fd closed = shutdown signal

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_addr.sin_addr, ip_buf, sizeof(ip_buf));
        std::string peer_ip(ip_buf);
        std::cout << "[listener] inbound from " << peer_ip << "\n";

        std::lock_guard<std::mutex> lock(threads_mutex);
        all_threads.emplace_back(inbound_peer_worker,
                                  peer_fd, std::ref(h),
                                  std::ref(manager), peer_ip);
    }
    std::cout << "[listener] exiting\n";
}