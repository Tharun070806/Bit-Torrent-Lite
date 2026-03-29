#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "./Parser/decoder.hpp"
#include "./utils/hasher.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " info <torrent_file>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "info") {
        std::string data = read_file(argv[2]);
        Torrent t = parse_torrent(data);

        print_torrent(t);
        std::string rawinfo = data.substr(t.info_start, t.info_end-t.info_start+1);
        std::cout<<sha1(rawinfo)<<std::endl;

        // ── Example: use the data however you want after this ──
        // t.announce       → tracker URL as std::string
        // t.name           → filename as std::string
        // t.length         → file size as int64_t
        // t.piece_length   → piece size as int64_t
        // t.pieces[0]      → first SHA1 hash as hex string
        // t.pieces.size()  → how many pieces
    }

    return 0;
}