#pragma once
#include "../parser/decoder.hpp"
#include <string>

void run_leecher(Torrent& t, std::string data_file, bool local_only);
