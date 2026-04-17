#include<iostream>
#include<string>
#include <cstdint>
std::uint32_t decode_uint32(const std::string& bytes, size_t offset = 0);
std::string encode_uint32(std::uint32_t value);
std::string generatePeerId(bool isLeech);