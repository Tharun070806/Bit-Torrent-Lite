#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "./Parser/decoder.hpp"
#include "./utils/hasher.hpp"
#include "./Requestor/requestor.hpp"
#include "./utils/socket_utils.hpp"


struct MyInfo {
    Torrent metadata;
    std::vector<string>PeerList; //properly formatted ipv4 format ig.
    std::string info_hash="";
    std::string PeerId; // Myown PeerId
    int Port; //Port That I am using to connect
    std::string TrackerHost; //Trackers hostname (no https, no path i.e //announce)
    int interval; 
    int mininterval;
    int seeders;
    int leachers;
};

    // ── This is The Structure of Other Data structures 

    //         struct Torrent {.         
    //     std::string announce;
    //     std::string created_by;
    //     std::string name;
    //     int64_t length = 0;
    //     int64_t piece_length = 0;
    //     int64_t info_start = 0;
    //     int64_t info_end = 0;
    //     std::string info_raw;
    //     std::vector<std::string> pieces;
    // };

    //     struct TrackerResponse {
    //     int64_t complete=0;
    //     int64_t incomplete=0;
    //     int interval=0;
    //     int mininterval=0;
    //     std::vector<std::string>peers; //properly formatted ipv4 format ig.
    // };

#define PORT 6881
#define BUFFER_SIZE 50000

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " info <torrent_file>\n";
        return 1;
    }

    std::string command = argv[1];

    MyInfo Details;
    Details.Port=PORT;

    if (command == "info") {

        std::string data = read_file(argv[2]);
        Torrent t = parse_torrent(data);
        Details.metadata=t;
        print_torrent(t);

        std::string rawinfo = data.substr(t.info_start, t.info_end-t.info_start+1);
        std::string info_hash=sha1(rawinfo,t);
        std::cout<<"the info hash is"<<info_hash<<std::endl;

        Details.info_hash=info_hash;

        std::string url=t.announce;
        std::string host=extract_host(url);
        Details.TrackerHost=host;
        std::string peer="01234567890123456789";
        Details.PeerId=peer;
        std::string path="/announce";

        std::string request=req_url(path,peer,t,host);

        int socket=connect_to(host,80,Details.Port);
        bool sent = send_all(socket, request.c_str(),request.size());

        char buffer [BUFFER_SIZE];
        std::string response="";
        recv_inf(response,buffer,socket);
        close_socket(socket);

        std::cout<<response<<std::endl;

        int pos=response.find("\r\n\r\n");
        std::string body = response.substr(pos+4);

        TrackerResponse ResponseFromTracker;
        size_t dumpos=0;
        parse_response_dict(body,dumpos,ResponseFromTracker);

        Details.seeders=ResponseFromTracker.complete;
        Details.leachers=ResponseFromTracker.incomplete;
        Details.PeerList=ResponseFromTracker.peers;

        print_response(ResponseFromTracker);
       
    }

    // if(command == "AnnounceTracker"){
    //     if(Details.info_hash.size()==0) {
    //         std::cerr << "Usage: " << argv[0] << " info <torrent_file>\n";
    //         return 1;
    //     }
    //     std::string path="/announce";
    //     std::string peer= Details.PeerId;
    //     Torrent t=Details.metadata;
    //     std::string url=Details.TrackerHost;
    //     std::string request=req_url(path,peer,t,url);

    //     int socket=connect_to(url,80,Details.Port);
    //     bool sent = send_all(socket, request.c_str(),request.size());

    //     char buffer [BUFFER_SIZE];
    //     std::string response="";
    //     recv_inf(response,buffer,socket);
    //     close_socket(socket);

    //     std::cout<<response<<std::endl;

    //     int pos=response.find("\r\n\r\n");
    //     std::string body = response.substr(pos+4);

    //     TrackerResponse ResponseFromTracker;
    //     size_t dumpos=0;
    //     parse_response_dict(body,dumpos,ResponseFromTracker);

    //     Details.seeders=ResponseFromTracker.complete;
    //     Details.leachers=ResponseFromTracker.incomplete;
    //     Details.PeerList=ResponseFromTracker.peers;

    //     print_response(ResponseFromTracker);
    // }

    // if (command == "handshake"){

    // }
    return 0;
};