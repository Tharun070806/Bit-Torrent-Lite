#pragma once

#include <string>

// Computes SHA1 hash of input and returns HEX string (40 chars)
std::string sha1(const std::string& input);
std::string extract_host(const std::string& url);
std::string sha1_withouthex(const std::string& input);
std::string sha1_withouthex(const char* input, int len);