
// PeerMessage.hpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <arpa/inet.h>   // htonl, ntohl
#include <sys/socket.h>  // recv, send
#include <cstring>       // memcpy
#include "../utils/socket_utils.hpp"

enum class MsgId : uint8_t {
    Choke         = 0,
    Unchoke       = 1,
    Interested    = 2,
    NotInterested = 3,
    Have          = 4,
    Bitfield      = 5,
    Request       = 6,
    Piece         = 7,
    Cancel        = 8,
    KeepAlive     = 255
};


struct PeerMessage {
    MsgId                id;
    std::vector<uint8_t> payload;
};


inline std::optional<PeerMessage> read_message(int fd) ;

inline std::vector<uint8_t> build_message(MsgId id,
                                           const std::vector<uint8_t>& payload={} ) ;

inline std::vector<uint8_t> build_keepalive();

inline std::vector<uint8_t> build_choke() ;
inline std::vector<uint8_t> build_unchoke() ;
inline std::vector<uint8_t> build_interested() ;
inline std::vector<uint8_t> build_not_interested();

inline std::vector<uint8_t> build_have(int piece_index) ;


inline std::vector<uint8_t> build_request(int piece_index,
                                           int block_offset,
                                           int block_length) ;

inline std::vector<uint8_t> build_piece_msg(int piece_index,
                                             int block_offset,
                                             const std::string& data) ;
