#include "hasher.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <iomanip>
#include <sstream>


#include "../Parser/decoder.hpp"

std::string sha1(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::string raw(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    
    // return raw;
    return hexconverter(raw);
}