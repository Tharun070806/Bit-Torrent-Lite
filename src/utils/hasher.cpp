
#include "../Parser/decoder.hpp"
#include "hasher.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <iomanip>
#include <sstream>



std::string sha1(const std::string& input,Torrent &t) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::string raw(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    t.info_raw=raw;
    
    // return raw;
    return hexconverter(raw);
}