#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "../utils/socket_utils.hpp"
#include "./Controller/Controller.cpp"
#include "./Peer.handshake.hpp"


struct PeerState {
    bool am_choking      = true;
    bool am_interested   = false;
    bool peer_choking    = true;
    bool peer_interested = false;
    std::vector<bool>          peer_bitfield;
    std::vector<BlockRequest>  pending_requests;
    std::vector<IncomingRequest> incoming_requests;
};



inline std::vector<uint8_t> build_piece_msg(int piece_index, int block_offset,
                                             const std::string& data) {
    std::vector<uint8_t> payload(8 + data.size());
    uint32_t pi  = htonl(piece_index);
    uint32_t off = htonl(block_offset);
    memcpy(payload.data(),     &pi,         4);
    memcpy(payload.data() + 4, &off,        4);
    memcpy(payload.data() + 8, data.data(), data.size());
    return build_message(MsgId::Piece, payload);
}

inline std::vector<uint8_t> build_have(int piece_index) {
    std::vector<uint8_t> payload(4);
    uint32_t pi = htonl(piece_index);
    memcpy(payload.data(), &pi, 4);
    return build_message(MsgId::Have, payload);
}

void request_next_blocks(int fd, PeerState& state, Manager& manager,
                         const std::string& peer_ip, int pipeline = 5) {
    if (manager.mode == Mode::SEEDER) return; // seeder never requests
    if (state.peer_choking)  return;
    if (!state.am_interested) return;

    while ((int)state.pending_requests.size() < pipeline) {
        auto req = manager.next_request(state.peer_bitfield);
        if (!req) break;

        auto msg = build_request(req->piece_index, req->block_offset,
                                 req->block_length);
        if (!send_bytes(fd, msg)) {
            manager.release_in_flight(req->piece_index, req->block_offset);
            break;
        }
        state.pending_requests.push_back(*req);
    }
}

void serve_incoming_requests(int fd, PeerState& state, Manager& manager,
                              const std::string& peer_ip) {
    if (state.am_choking) return;

    for (auto it = state.incoming_requests.begin();
         it != state.incoming_requests.end(); ) {

        std::string block = manager.get_block(it->piece_index,
                                               it->block_offset,
                                               it->block_length);
        if (block.empty()) { ++it; continue; }

        auto msg = build_piece_msg(it->piece_index, it->block_offset, block);
        if (!send_bytes(fd, msg)) break;

        std::cout << "[" << peer_ip << "] served piece="
                  << it->piece_index << " offset=" << it->block_offset << "\n";
        it = state.incoming_requests.erase(it);
    }
}

void flush_haves(int fd, int peer_id, Manager& manager,
                 const std::string& peer_ip) {
    for (int pi : manager.drain_haves(peer_id)) {
        if (!send_bytes(fd, build_have(pi))) return;
        std::cout << "[" << peer_ip << "] sent Have " << pi << "\n";
    }
}

// ── Core message loop — shared by inbound and outbound ────────────────────
void run_peer_loop(int fd, int peer_id, PeerState& state,
                   Manager& manager, const std::string& peer_ip) {

    // seeder never wants anything
    bool should_express_interest = (manager.mode == Mode::LEECHER);

    // send our bitfield
    {
        auto bf     = manager.get_bitfield();
        auto bf_msg = build_message(MsgId::Bitfield, bf);
        send_bytes(fd, bf_msg);
    }

    // leecher signals interest upfront
    if (should_express_interest) {
        send_bytes(fd, build_message(MsgId::Interested));
        state.am_interested = true;
    }

    while (true) {
        // exit condition depends on mode
        if (manager.mode == Mode::LEECHER && manager.all_done()) break;

        flush_haves(fd, peer_id, manager, peer_ip);
        serve_incoming_requests(fd, state, manager, peer_ip);

        auto msg_opt = read_message(fd);
        if (!msg_opt) {
            std::cerr << "[" << peer_ip << "] disconnected\n";
            break;
        }
        auto& msg = *msg_opt;

        switch (msg.id) {

            case MsgId::KeepAlive: {
                uint32_t zero = 0;
                send(fd, &zero, 4, 0);
                break;
            }

            case MsgId::Choke:
                state.peer_choking = true;
                for (auto& r : state.pending_requests)
                    manager.release_in_flight(r.piece_index, r.block_offset);
                state.pending_requests.clear();
                break;

            case MsgId::Unchoke:
                state.peer_choking = false;
                request_next_blocks(fd, state, manager, peer_ip);
                break;

            case MsgId::Interested:
                state.peer_interested = true;
                state.am_choking      = false;
                send_bytes(fd, build_message(MsgId::Unchoke));
                serve_incoming_requests(fd, state, manager, peer_ip);
                break;

            case MsgId::NotInterested:
                state.peer_interested = false;
                state.am_choking      = true;
                send_bytes(fd, build_message(MsgId::Choke));
                break;

            case MsgId::Have: {
                if (msg.payload.size() < 4) break;
                uint32_t pi_be;
                memcpy(&pi_be, msg.payload.data(), 4);
                int piece = ntohl(pi_be);
                if (piece < manager.total_pieces)
                    state.peer_bitfield[piece] = true;

                if (manager.mode == Mode::LEECHER && !state.am_interested) {
                    auto req = manager.next_request(state.peer_bitfield);
                    if (req) {
                        manager.release_in_flight(req->piece_index,
                                                  req->block_offset);
                        send_bytes(fd, build_message(MsgId::Interested));
                        state.am_interested = true;
                    }
                }
                break;
            }

            case MsgId::Bitfield: {
                for (int i = 0; i < manager.total_pieces; i++) {
                    int byte_idx = i / 8;
                    int bit      = 7 - (i % 8);
                    if (byte_idx < (int)msg.payload.size())
                        state.peer_bitfield[i] =
                            (msg.payload[byte_idx] >> bit) & 1;
                }
                std::cout << "[" << peer_ip << "] got bitfield\n";

                if (manager.mode == Mode::LEECHER) {
                    auto req = manager.next_request(state.peer_bitfield);
                    if (req) {
                        manager.release_in_flight(req->piece_index,
                                                  req->block_offset);
                        if (!state.am_interested) {
                            send_bytes(fd, build_message(MsgId::Interested));
                            state.am_interested = true;
                        }
                    } else {
                        send_bytes(fd, build_message(MsgId::NotInterested));
                        state.am_interested = false;
                    }
                }
                break;
            }

            case MsgId::Request: {
                if (msg.payload.size() < 12) break;
                uint32_t pi_be, off_be, len_be;
                memcpy(&pi_be,  msg.payload.data(),     4);
                memcpy(&off_be, msg.payload.data() + 4, 4);
                memcpy(&len_be, msg.payload.data() + 8, 4);
                IncomingRequest req;
                req.piece_index  = ntohl(pi_be);
                req.block_offset = ntohl(off_be);
                req.block_length = ntohl(len_be);

                if (state.am_choking) break;
                state.incoming_requests.push_back(req);
                serve_incoming_requests(fd, state, manager, peer_ip);
                break;
            }

            case MsgId::Piece: {
                if (manager.mode == Mode::SEEDER) break; // seeder ignores
                if (msg.payload.size() < 8) break;

                uint32_t pi_be, off_be;
                memcpy(&pi_be,  msg.payload.data(),     4);
                memcpy(&off_be, msg.payload.data() + 4, 4);
                int piece_index  = ntohl(pi_be);
                int block_offset = ntohl(off_be);

                std::string block_data(
                    (char*)msg.payload.data() + 8,
                    msg.payload.size() - 8
                );

                state.pending_requests.erase(
                    std::remove_if(state.pending_requests.begin(),
                                   state.pending_requests.end(),
                                   [&](const BlockRequest& r) {
                                       return r.piece_index  == piece_index &&
                                              r.block_offset == block_offset;
                                   }),
                    state.pending_requests.end()
                );

                bool piece_done = manager.block_received(
                    piece_index, block_offset, block_data);
                if (piece_done)
                    std::cout << "[" << peer_ip << "] piece "
                              << piece_index << " done!\n";

                request_next_blocks(fd, state, manager, peer_ip);
                break;
            }

            case MsgId::Cancel: {
                if (msg.payload.size() < 12) break;
                uint32_t pi_be, off_be;
                memcpy(&pi_be,  msg.payload.data(),     4);
                memcpy(&off_be, msg.payload.data() + 4, 4);
                int piece_index  = ntohl(pi_be);
                int block_offset = ntohl(off_be);
                state.incoming_requests.erase(
                    std::remove_if(state.incoming_requests.begin(),
                                   state.incoming_requests.end(),
                                   [&](const IncomingRequest& r) {
                                       return r.piece_index  == piece_index &&
                                              r.block_offset == block_offset;
                                   }),
                    state.incoming_requests.end()
                );
                break;
            }

            default:
                break;
        }
    }
}

// ── Outbound peer (we initiate connection) ────────────────────────────────
void peer_worker(int fd, const handshake& h, Manager& manager,
                 const std::string& peer_ip, int peer_port) {

    int peer_id = manager.register_peer();
    PeerState state;
    state.peer_bitfield.resize(manager.total_pieces, false);

    // send handshake
    unsigned char hs[68]; int ptr = 0;
    hs[ptr++] = h.byte19;
    memcpy(hs + ptr, h.protocol,  19); ptr += 19;
    memcpy(hs + ptr, h.zero,       8); ptr += 8;
    memcpy(hs + ptr, h.info_hash, 20); ptr += 20;
    memcpy(hs + ptr, h.peer_id,   20);
    if (send(fd, hs, 68, 0) != 68) goto cleanup;

    // recv handshake
    {
        unsigned char hr[68]; size_t got = 0;
        while (got < 68) {
            ssize_t r = recv(fd, hr + got, 68 - got, 0);
            if (r <= 0) goto cleanup;
            got += r;
        }
        if (memcmp(hr + 28, h.info_hash, 20) != 0) goto cleanup;
        std::cout << "[" << peer_ip << "] handshake OK\n";
    }

    run_peer_loop(fd, peer_id, state, manager, peer_ip);

cleanup:
    for (auto& r : state.pending_requests)
        manager.release_in_flight(r.piece_index, r.block_offset);
    manager.unregister_peer(peer_id);
    close(fd);
}

// ── Inbound peer (they initiate connection) ───────────────────────────────
void inbound_peer_worker(int fd, const handshake& h, Manager& manager,
                         const std::string& peer_ip) {

    int peer_id = manager.register_peer();
    PeerState state;
    state.peer_bitfield.resize(manager.total_pieces, false);

    // recv their handshake first
    unsigned char hr[68]; size_t got = 0;
    while (got < 68) {
        ssize_t r = recv(fd, hr + got, 68 - got, 0);
        if (r <= 0) goto cleanup;
        got += r;
    }
    if (hr[0] != 19) goto cleanup;
    if (memcmp(hr + 1, "BitTorrent protocol", 19) != 0) goto cleanup;
    if (memcmp(hr + 28, h.info_hash, 20) != 0) goto cleanup;

    // send our handshake back
    {
        unsigned char hs[68]; int ptr = 0;
        hs[ptr++] = h.byte19;
        memcpy(hs + ptr, h.protocol,  19); ptr += 19;
        memcpy(hs + ptr, h.zero,       8); ptr += 8;
        memcpy(hs + ptr, h.info_hash, 20); ptr += 20;
        memcpy(hs + ptr, h.peer_id,   20);
        if (send(fd, hs, 68, 0) != 68) goto cleanup;
    }

    std::cout << "[inbound][" << peer_ip << "] handshake OK\n";
    run_peer_loop(fd, peer_id, state, manager, peer_ip);

cleanup:
    for (auto& r : state.pending_requests)
        manager.release_in_flight(r.piece_index, r.block_offset);
    manager.unregister_peer(peer_id);
    close(fd);
}

