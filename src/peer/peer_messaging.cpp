
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <arpa/inet.h>   // htonl, ntohl
#include <sys/socket.h>  // recv, send
#include <cstring> 
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

inline std::optional<PeerMessage> read_message(int fd) {

    std::cout<<"Reading Message.."<<std::endl;

    uint32_t len_be = 0;
    if (!recv_exact(fd, &len_be, 4)) return std::nullopt;

    uint32_t len = ntohl(len_be);

   
    if (len == 0) {
        return PeerMessage{MsgId::KeepAlive, {}};
    }


    uint8_t id = 0;
    if (!recv_exact(fd, &id, 1)) return std::nullopt;

  
    std::vector<uint8_t> payload;
    if (len > 1) {
        payload.resize(len - 1);
        if (!recv_exact(fd, payload.data(), len - 1)) return std::nullopt;
    }

    return PeerMessage{(MsgId)id, std::move(payload)};
}

inline std::vector<uint8_t> build_message(MsgId id,
                                           const std::vector<uint8_t>& payload = {}) {
    std::cout<<"building message(Any)"<<std::endl;
    // length field = 1 byte for id + however many payload bytes
    uint32_t len    = 1 + (uint32_t)payload.size();
    uint32_t len_be = htonl(len);   

    // allocate full buffer: 4 (length) + 1 (id) + payload
    std::vector<uint8_t> msg(4 + 1 + payload.size());

    memcpy(msg.data(),     &len_be, 4);          
    msg[4] = (uint8_t)id;                        
    if (!payload.empty())
        memcpy(msg.data() + 5, payload.data(), payload.size()); 

    return msg;
}


inline std::vector<uint8_t> build_keepalive() {
    return {0x00, 0x00, 0x00, 0x00};
}

// Choke, Unchoke, Interested, NotInterested: no payload
inline std::vector<uint8_t> build_choke() {
    return build_message(MsgId::Choke);
}
inline std::vector<uint8_t> build_unchoke() {
    return build_message(MsgId::Unchoke);
}
inline std::vector<uint8_t> build_interested() {
    std::cout<<"Building interested.."<<std::endl;
    return build_message(MsgId::Interested);
}
inline std::vector<uint8_t> build_not_interested() {
    return build_message(MsgId::NotInterested);
}


inline std::vector<uint8_t> build_have(int piece_index) {
    std::vector<uint8_t> payload(4);
    uint32_t pi = htonl((uint32_t)piece_index);
    memcpy(payload.data(), &pi, 4);
    return build_message(MsgId::Have, payload);
}


inline std::vector<uint8_t> build_request(int piece_index,
                                           int block_offset,
                                           int block_length) {
    std::cout<<"Building request.."<<piece_index<<" "<<block_offset<<std::endl;
    std::vector<uint8_t> payload(12);  

    uint32_t pi  = htonl((uint32_t)piece_index);
    uint32_t off = htonl((uint32_t)block_offset);
    uint32_t len = htonl((uint32_t)block_length);

    memcpy(payload.data() + 0, &pi,  4);   
    memcpy(payload.data() + 4, &off, 4);   
    memcpy(payload.data() + 8, &len, 4);   

    return build_message(MsgId::Request, payload);
}

// Piece: payload = [4 piece_index][4 block_offset][data...]
// Sent in response to a Request — contains the actual block data
//
// This is the message that carries file content between peers

inline std::vector<uint8_t> build_piece_msg(int piece_index,
                                             int block_offset,
                                             const std::string& data) {
                            
                                                std::cout<<"Building piece.."<<std::endl;
    std::vector<uint8_t> payload(8 + data.size());

    uint32_t pi  = htonl((uint32_t)piece_index);
    uint32_t off = htonl((uint32_t)block_offset);

    memcpy(payload.data(),     &pi,         4);
    memcpy(payload.data() + 4, &off,        4);
    memcpy(payload.data() + 8, data.data(), data.size());

    return build_message(MsgId::Piece, payload);
}
