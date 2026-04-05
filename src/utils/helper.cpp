#include "helper.hpp"

std::string encode_uint32(std::uint32_t value) {
    std::string bytes(4, '\0');
    bytes[0] = static_cast<char>((value >> 24) & 0xFF);
    bytes[1] = static_cast<char>((value >> 16) & 0xFF);
    bytes[2] = static_cast<char>((value >> 8) & 0xFF);
    bytes[3] = static_cast<char>(value & 0xFF);
    return bytes;
}

std::uint32_t decode_uint32(const std::string& bytes, size_t offset = 0) {
    if (offset + 4 > bytes.size()) {
        throw std::runtime_error("Invalid uint32 encoding");
    }

    return
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) << 24) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 16) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 8) |
        static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3]));
}

std::string generatePeerId(bool isLeech) {
    const int TOTAL_LENGTH = 20;

    std::string prefix = isLeech ? "LEECH" : "SEED";
    std::string charset = "0123456789";

    std::string peer_id = prefix;

    while (peer_id.size() < TOTAL_LENGTH) {
        int index = rand() % charset.size();  // pick random digit
        peer_id += charset[index];
    }

    return peer_id;
}