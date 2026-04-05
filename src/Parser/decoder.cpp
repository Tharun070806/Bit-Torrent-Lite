#include "decoder.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include<unordered_map>
// ─── File reader 



std::string read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// ─── Bencode parser helpers 

std::string read_string(const std::string& data, size_t& pos) {
    size_t colon = data.find(':', pos);
    if(colon==std::string::npos){
        throw std::runtime_error(
            "Cant find string End — pos=" + std::to_string(pos) + 
            " char=" + std::to_string((int)data[pos])
        );
    }
    int len = std::stoi(data.substr(pos, colon - pos));
    pos = colon + 1;
    std::string result = data.substr(pos, len);
    pos += len;
    return result;
}

int64_t read_integer(const std::string& data, size_t& pos) {
    pos++; // skip 'i'
    size_t end = data.find('e', pos);
    if(end==std::string::npos){
        throw std::runtime_error("Cant find int End");
    }
    int64_t result = std::stoll(data.substr(pos, end - pos));
    pos = end + 1;
    return result;
}

void skip_value(const std::string& data, size_t& pos) {
   
    if (data[pos] == 'i') {
        pos++;
        pos = data.find('e', pos) + 1;
        if(pos==std::string::npos){
        throw std::runtime_error("Cant find int End");
    }
    } else if (std::isdigit(data[pos])) {
        read_string(data, pos);
    } else if (data[pos] == 'l' || data[pos] == 'd') {
        pos++;
        while (data[pos] != 'e') skip_value(data, pos);
        pos++;
    }
}



std::string hexconverter(const std::string &hash){
        // convert each hash to hex string for easy use
        std::string hex;
        for (unsigned char c : hash) {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02x", c);
                    hex += buf;
        }
        return hex;
}

std::string decconverter(const std::string &hash){
        int byte=std::stoi(hash,nullptr,16);
        return std::to_string(byte);
}

void parse_info_dict(const std::string& data, size_t& pos, Torrent& t) {
    pos++; // skip 'd'
    while (pos < data.size() && data[pos] != 'e') {
        std::string key = read_string(data, pos);

        if (key == "length") {
            t.length = read_integer(data, pos);
        } else if (key == "name") {
            t.name = read_string(data, pos);
        } else if (key == "piece length") {
            t.piece_length = read_integer(data, pos);
        } else if (key == "pieces") {
            // pieces is one big binary string — split into 20-byte SHA1 hashes
            std::string raw = read_string(data, pos);
            for (size_t i = 0; i + 20 <= raw.size(); i += 20) {
                std::string hash = raw.substr(i, 20);

                // convert each hash to hex string for easy use
                
                std::string hex=hexconverter(hash);
                // for (unsigned char c : hash) {
                //     char buf[3];
                //     snprintf(buf, sizeof(buf), "%02x", c);
                //     hex += buf;
                // }
                t.pieces.push_back(hex);
            }
        } else {
            skip_value(data, pos);
        }
    }
    pos++; // skip 'e'
}

// ─── Parse the whole torrent file

Torrent parse_torrent(const std::string& data) {
    Torrent t;
    size_t pos = 0;
    pos++; // skip outer 'd'

    while (pos < data.size() && data[pos] != 'e') {
        std::string key = read_string(data, pos);

        if (key == "announce") {
            t.announce = read_string(data, pos);
        } else if (key == "created by") {
            t.created_by = read_string(data, pos);
        } else if (key == "info") {
            t.info_start=pos;
            parse_info_dict(data, pos, t);
            t.info_end=pos-1;
        } else {
            skip_value(data, pos);
        }
    }

    return t;
}

void parse_response_dict(const std::string& data, size_t& pos, TrackerResponse &t) {
    pos++; // skip 'd'
    while (pos < data.size() && data[pos] != 'e') {
        std::string key = read_string(data, pos);

        if (key == "complete") {
            t.complete = read_integer(data, pos);
        } else if (key == "incomplete") {
            t.incomplete = read_integer(data, pos);
        } else if (key == "interval") {
            t.interval = read_integer(data, pos);
        } 
        else if(key=="min interval"){
            t.mininterval=read_integer(data,pos);
        }
        else if (key == "peers") {
               std::string raw = read_string(data, pos);
                for (size_t i = 0; i + 6 <= raw.size(); i += 6) {
                    std::string orig  = raw.substr(i, 6);
                    unsigned char a=orig[0];
                    unsigned char b=orig[1];
                    unsigned char c=orig[2];
                    unsigned char d=orig[3];

                    unsigned char e=orig[4];
                    unsigned char f=orig[5];
                    unsigned short port=e<<8 | f;

                    std::string ip=std::to_string(a)+"."+std::to_string(b)+"."+std::to_string(c)+"."+std::to_string(d)+":"+std::to_string(port);
                    t.peers.push_back(ip);
                
                }
        } else {
            skip_value(data, pos);
        }
    }
    pos++; // skip 'e'
}

// ─── Print everything 

void print_torrent(const Torrent& t) {
    std::cout << "Tracker URL  : " << t.announce      << "\n";
    std::cout << "Created by   : " << t.created_by    << "\n";
    std::cout << "File name    : " << t.name          << "\n";
    std::cout << "File size    : " << t.length        << " bytes\n";
    std::cout << "Piece length : " << t.piece_length  << " bytes\n";
    std::cout << "Num pieces   : " << t.pieces.size() << "\n";
    std::cout << "Pieces (SHA1 hashes):\n";
    for (size_t i = 0; i < t.pieces.size(); i++) {
        std::cout << "  [" << i + 1 << "] " << t.pieces[i] << "\n";
    }
}
void print_response(const TrackerResponse& t) {
    std::cout << "complete  : " << t.complete      << "\n";
    std::cout << "incomplete   : " << t.incomplete    << "\n";
    std::cout << "interval    : " << t.interval          << "\n";
    std::cout << "min interval    : " << t.mininterval        << " bytes\n";

    for (size_t i = 0; i < t.peers.size(); i++) {
        std::cout << "  [" << i + 1 << "] " << t.peers[i] << "\n";
    }
}

