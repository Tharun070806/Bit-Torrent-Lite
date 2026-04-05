#pragma once
#include<iostream>
#include<string>
#include<vector>
#include <mutex>
#include<unordered_set>
#include <condition_variable>
#include <algorithm>
#include<optional>
#include <fstream>
#include<unordered_map>
#include<fstream>
#include <cstdio>   
#include <unistd.h>   
#include <fcntl.h>
#include "../utils/hasher.hpp"

enum class Mode {
    LEECHER,
    SEEDER
};

struct IncomingRequest {
    int piece_index;
    int block_offset;
    int block_length;
};


struct BlockRequest {
    int piece_index;
    int block_offset;
    int block_length;
};

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <mutex>
#include <condition_variable>

enum class Mode {
    LEECHER,
    SEEDER
};

struct BlockRequest {
    int piece_index;
    int block_offset;
    int block_length;
};

struct IncomingRequest {
    int piece_index;
    int block_offset;
    int block_length;
};

class Controller {
    public:
    
    Mode mode;
    std::string filename;
    int64_t length;
    int total_pieces;
    int block_length = 16384;
    int piece_length;
    int crnt_peer = 0;

    std::mutex mtx;
    std::condition_variable cv;

    std::vector<std::string> piece_hash;
    std::vector<std::unordered_set<int>> inflight;
    std::vector<int> piece_status;
    std::vector<std::unordered_map<int, std::string>> piece_blocks;
    std::unordered_map<int, std::vector<int>> send_haves;
    std::vector<std::string> assembled_pieces;


    Controller(Mode mode,
               int total_pieces,
               int64_t length,
               int piece_length,
               std::vector<std::string>& piece_hash,
               std::string filename = "");

    int piece_length_for(int i);
    int count_blocks(int piece_index);

    std::optional<BlockRequest> next_request(const std::vector<int> bit_field);

    bool all_done();
    bool seederCheck();

    bool received_block(int piece_index, int block_offset, const std::string &data);

    void write_to_disk(int piece_index, std::string& piece);
    std::string get_block(int piece_index, int blockoffset);

    void release_in_flight(int piece_index, int block_offset);

    int register_peer();
    void unregister_peer(int peer_id);
    std::vector<int> drain_haves(int peer_id);
};

