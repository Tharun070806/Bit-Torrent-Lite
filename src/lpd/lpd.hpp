#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "../controller/controller.hpp"
#include "../peer/peer_handshake.hpp"

// Broadcaster sends out periodic UDP packets
void lpd_broadcast_thread(std::string info_hash_hex, int tcp_port, std::string my_cookie);

// Listener parses incoming UDP UDP packets and directly triggers TCP connects
void lpd_listen_thread(std::string my_info_hash, std::string my_cookie, const handshake& h,
                       Controller& manager, std::vector<std::thread>& all_threads, std::mutex& mtx);
