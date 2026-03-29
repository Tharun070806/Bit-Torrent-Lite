#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct Torrent {
    std::string announce;
    std::string created_by;
    std::string name;

    int64_t length = 0;
    int64_t piece_length = 0;

    int64_t info_start = 0;
    int64_t info_end = 0;

    std::vector<std::string> pieces;
};

std::string read_file(const std::string& filename);

std::string read_string(const std::string& data, size_t& pos);
int64_t read_integer(const std::string& data, size_t& pos);
void skip_value(const std::string& data, size_t& pos);

void parse_info_dict(const std::string& data, size_t& pos, Torrent& t);
Torrent parse_torrent(const std::string& data);

std::string hexconverter(const std::string& hash);

void print_torrent(const Torrent& t);