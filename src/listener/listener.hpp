#pragma once
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../controller/controller.hpp"
#include "../peer/peer.hpp"
#include "../peer/peer_handshake.hpp"

void listener_thread(int listen_fd, const handshake& h, Controller& manager,
                     std::vector<std::thread>& all_threads,
                     std::mutex& threads_mutex);