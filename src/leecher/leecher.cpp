#include "leecher.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include "../utils/hasher.hpp"
#include "../requestor/requestor.hpp"
#include "../utils/socket_utils.hpp"
#include "../peer/peer_handshake.hpp"
#include "../utils/helper.hpp"
#include "../listener/listener.hpp"
#include "../lpd/lpd.hpp"
#include "../peer/peer.hpp"

#define PORT 6882
#define BUFFER_SIZE 50000

void run_leecher(Torrent& t, std::string data_file, bool local_only) {
    std::string info_hash = sha1(t.info_raw);
    std::cout << info_hash << std::endl;
    std::string peer = generatePeerId(true);
    
    std::vector<std::string> PeerList;

    if (!local_only) {
        std::string raw_host_str = extract_host(t.announce);
    std::string host = raw_host_str;
    int tracker_port = 80;

    size_t colon_pos = raw_host_str.find(':');
    if (colon_pos != std::string::npos) {
        tracker_port = std::stoi(raw_host_str.substr(colon_pos + 1));
        host = raw_host_str.substr(0, colon_pos);
    }

    std::string path = "/";
    size_t protocol_end = t.announce.find("://");
    if (protocol_end != std::string::npos) {
        size_t slash = t.announce.find('/', protocol_end + 3);
        if (slash != std::string::npos) {
            path = t.announce.substr(slash);
        }
    }

    std::string request = req_url(path, peer, t, host, tracker_port, true, PORT);
    std::cout << "Connecting to clean host: [" << host << "] on port: " << tracker_port << "\n" << "Path: " << path << std::endl;

    int socket = connect_to_specific(host, tracker_port, PORT);
    bool sent = send_all(socket, request.c_str(), request.size());

    char buffer[BUFFER_SIZE];
    std::string response = recv_inf(socket);
    close_socket(socket);

    std::cout << response << std::endl;

    int pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) {
        std::cerr << "[leecher] bad tracker response\n";
        return;
    }
    std::string body = response.substr(pos + 4);

    TrackerResponse ResponseFromTracker;
    size_t dumpos = 0;
        parse_response_dict(body, dumpos, ResponseFromTracker);
        PeerList = ResponseFromTracker.peers;

        print_response(ResponseFromTracker);
    } else {
        std::cout << "[leecher] Operating in purely local discovery mode (Tracker bypassed).\n";
    }

    int listening_socket = create_server_socket(PORT);

    Controller manager(Mode::LEECHER, t.pieces.size(), (int)t.piece_length, t.length, t.pieces, data_file);

    std::vector<std::thread> all_threads;
    std::mutex threads_mutex;

    handshake h;
    info_hashadder(h, t);
    peer_idadder(h, peer);

    std::thread listener(listener_thread, listening_socket, std::ref(h), std::ref(manager), std::ref(all_threads), std::ref(threads_mutex));

    std::string lpd_cookie = "LPD_" + std::to_string((long long)time(nullptr));
    std::thread lpd_broadcaster(lpd_broadcast_thread, info_hash, PORT, lpd_cookie);
    std::thread lpd_listener_th(lpd_listen_thread, info_hash, lpd_cookie, std::ref(h), std::ref(manager), std::ref(all_threads), std::ref(threads_mutex));
    lpd_broadcaster.detach();
    lpd_listener_th.detach();

    PeerList.push_back("127.0.0.1:6882");
    
    if(PeerList.size() == 0 && !local_only) {
        std::cerr << "[leecher] no peers from tracker" << std::endl;
        close(listening_socket);
        listener.join();
        return;
    }
    
    std::vector<int> SocketList;
    for(auto peerip : PeerList) {
        int sep = peerip.find(':');
        if(sep == std::string::npos) continue;
        std::string ip = peerip.substr(0, sep);
        int peer_port = std::stoi(peerip.substr(sep + 1));
        if (ip.empty() || peer_port <= 0) continue;
        
        int fd = connect_to(ip, peer_port);
        SocketList.push_back(fd);
        if (fd < 0) {
            std::cerr << "[leecher] cannot connect to " << ip << "\n";
            continue;
        }
        std::cout << "[leecher] connected to " << ip << ":" << peer_port << std::endl;
        
        std::lock_guard<std::mutex> lock(threads_mutex);
        all_threads.emplace_back(peer_worker, fd, std::ref(h), std::ref(manager), ip, peer_port);
    }

    manager.wait_until_done();
    std::cout << "[leecher] download complete! saved to " << t.name << "\n";

    close(listening_socket);
    for(auto id : SocketList) {
        shutdown(id, SHUT_RDWR); 
        close(id);
    }
    
    listener.join();
    
    std::lock_guard<std::mutex> lock(threads_mutex);
    for (auto& thr : all_threads) {
        if (thr.joinable()) thr.join();
    }
}
