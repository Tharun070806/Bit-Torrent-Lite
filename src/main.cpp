#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "./Parser/decoder.hpp"
#include "./utils/hasher.hpp"
#include "./Requestor/requestor.hpp"
#include "./utils/socket_utils.hpp"

#define PORT 6881
#define BUFFER_SIZE 50000

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
        std::string info_hash=sha1(rawinfo,t);
        std::cout<<"the info hash is"<<info_hash<<std::endl;

        // ── Example: use the data however you want after this ──
        // t.announce       → tracker URL as std::string
        // t.name           → filename as std::string
        // t.length         → file size as int64_t
        // t.piece_length   → piece size as int64_t
        // t.pieces[0]      → first SHA1 hash as hex string
        // t.pieces.size()  → how many pieces

        std::string url="bittorrent-test-tracker.codecrafters.io";
        std::string peer="01234567890123456789";
        std::string path="/announce";

        std::string request=req_url(path,peer,t,url);

        int socket=connect_to(url,80,6881);
        bool sent = send_all(socket, request.c_str(),request.size());

        char buffer [BUFFER_SIZE];
        std::string response="";
        recv_inf(response,buffer,socket);

        std::cout<<response<<std::endl;
        close_socket(socket);
        int pos=response.find("\r\n\r\n");
        std::string body = response.substr(pos+4);
        Peerlist peers;
        size_t dumpos=0;
        parse_response_dict(body,dumpos,peers);
        print_response(peers);

        



    }

    return 0;
};