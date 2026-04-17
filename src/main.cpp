#include <iostream>
#include <fstream>
#include <string>
#include "./parser/decoder.hpp"
#include "./utils/hasher.hpp"
#include "./leecher/leecher.hpp"
#include "./seeder/seeder.hpp"

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " info <torrent_file>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "download") {
        command = "leecher";
    }

    if ((command == "seeder" || command == "seed") && argc < 4) {
        std::cerr << "seed/seeder requires: <torrent_file> <data_file>\n";
        return 1;
    }

    if(command == "encode"){
        std::string trackerURL="http://tracker.mywaifu.best:6969/announce";
        std::string filename=argv[2];
        create_torrent(filename, trackerURL);
        return 0;
    }

    // Determine modes and arguments
    bool local_only = false;
    int file_index = 2; // Default location for older commands

    if (command == "leecher" || command == "seeder" || command == "seed" || command == "download") {
        if (argc >= 4) {
            std::string mode_str = argv[2];
            if (mode_str == "local" || mode_str == "general") {
                local_only = (mode_str == "local");
                file_index = 3;
            }
        }
    }

    if (argc <= file_index) {
        std::cerr << "Missing required torrent file argument." << std::endl;
        return 1;
    }

    std::string data = read_file(argv[file_index]);
    Torrent t = parse_torrent(data);

    print_torrent(t);

    // Compute raw info
    t.info_raw = data.substr(t.info_start, t.info_end - t.info_start + 1);

    if (command == "info" || command == "decode") {
        print_torrent(t);
        return 0; 
    }

    if (command == "leecher" || command == "download") {
        run_leecher(t, t.name, local_only);
    } else if (command == "seeder" || command == "seed") {
        std::string data_file = (argc > file_index + 1) ? argv[file_index + 1] : t.name;
        run_seeder(t, data_file, local_only);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}