#pragma once
#include<string>
#include<iomanip>

std::string url_encode(const std::string& raw);

std::string req_url(std::string& path,std::string &peer_id, Torrent&t, std::string url);