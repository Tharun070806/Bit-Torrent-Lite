
#include "../parser/decoder.hpp"
#include "hasher.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <iomanip>
#include <sstream>



std::string sha1(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::string raw(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    return hexconverter(raw);
}



std::string sha1_withouthex(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::string raw(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    return raw;
}

std::string sha1_withouthex(const char* input, int len) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input), len , hash);
    std::string raw(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    return raw;
}



std::string extract_host(const std::string& url) {
    size_t start = url.find("://");
    
    if (start == std::string::npos) return ""; // invalid URL
    start += 3; // skip "://"

    size_t end = url.find('/', start);

    if (end == std::string::npos) {
        return url.substr(start); // no path, only host
    }

    return url.substr(start, end - start);
}