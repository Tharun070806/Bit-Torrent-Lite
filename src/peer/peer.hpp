#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "../controller/controller.hpp"
#include "./peer_handshake.hpp"
#include "./peer_messaging.cpp"


// ---------------- Peer State ----------------

struct PeerState {
    bool am_choking      = true;
    bool am_interested   = false;
    bool peer_choking    = true;
    bool peer_interested = false;

    std::vector<bool> peer_bitfield;
    std::vector<BlockRequest> pending_requests;
    std::vector<IncomingRequest> incoming_requests;
};

// ---------------- Core Functions ----------------

// Request pipeline
void request_next_blocks(
    int fd,
    PeerState& state,
    Controller& manager,
    const std::string& peer_ip,
    int pipeline = 5
);

// Serve upload requests
void serve_incoming_requests(
    int fd,
    PeerState& state,
    Controller& manager,
    const std::string& peer_ip
);

// Send HAVE messages
void flush_haves(
    int fd,
    int peer_id,
    Controller& manager,
    const std::string& peer_ip
);

// Main loop
void run_peer_loop(
    int fd,
    int peer_id,
    PeerState& state,
    Controller& manager,
    const std::string& peer_ip
);

// Outbound peer
void peer_worker(
    int fd,
    const handshake& h,
    Controller& manager,
    const std::string& peer_ip,
    int peer_port
);

// Inbound peer
void inbound_peer_worker(
    int fd,
    const handshake& h,
    Controller& manager,
    const std::string& peer_ip
);