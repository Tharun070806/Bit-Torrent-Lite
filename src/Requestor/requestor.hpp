#pragma once
#include<string>
#include<iomanip>
#include "../parser/decoder.hpp"
#include "../utils/hasher.hpp"

std::string url_encode(const std::string& raw);

std::string req_url(std::string& path,std::string &peer_id, Torrent&t, std::string host, int tracker_port, bool leecher, int port);