#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct TrackerResponse {
    int64_t complete=0;
    int64_t incomplete=0;
    int interval=0;
    int mininterval=0;
    std::vector<std::string>peers;
};
struct Torrent {
    std::string announce;
    std::string created_by;
    std::string name;

    int64_t length = 0;
    int64_t piece_length = 0;

    int64_t info_start = 0;
    int64_t info_end = 0;

    std::string info_raw;

    std::vector<std::string> pieces;
};

std::string read_file(const std::string& filename);

std::string read_string(const std::string& data, size_t& pos);
int64_t read_integer(const std::string& data, size_t& pos);
void skip_value(const std::string& data, size_t& pos);

void parse_info_dict(const std::string& data, size_t& pos, Torrent& t);
Torrent parse_torrent(const std::string& data);
void create_torrent(const std::string& filename, const std::string& tracker_url);
std::string hexconverter(const std::string& hash);

void print_torrent(const Torrent& t);
void parse_response_dict(const std::string& data, size_t& pos, TrackerResponse &t);
void print_response(const TrackerResponse& t);