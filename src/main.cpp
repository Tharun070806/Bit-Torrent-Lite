#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include "./Parser/decoder.hpp"
#include "./utils/hasher.hpp"
#include "./Requestor/requestor.hpp"
#include "./utils/socket_utils.hpp"
#include "./Peer.Messaging/Peer.handshake.hpp"
#include "./utils/helper.hpp"


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

    if(command!="leecher" && command!="seeder") {
        std::cerr << "Give proper Command"<<endl;
    }

    if(command=="seeder" && argc<4){
        std::cerr << "seed requires: <torrent_file> <data_file>\n";
        return 1;
    }

        std::string data = read_file(argv[2]);
        Torrent t = parse_torrent(data);
        Details.metadata=t;

        //printing torrent details like tracker, peiece etc.. 

        print_torrent(t);

        std::string rawinfo = data.substr(t.info_start, t.info_end-t.info_start+1);
        t.info_raw=rawinfo;
        std::string info_hash=sha1(rawinfo);
        std::string peer= (command=="leecher") ? generatePeerId(true): generatePeerId(false);
        std::string host=extract_host(t.announce);
        std::string path="/announce";
        std::string request=req_url(path,peer,t,host);

        int socket=connect_to(host,80);
        bool sent = send_all(socket, request.c_str(),request.size());

        char buffer [BUFFER_SIZE];
        std::string response=recv_inf(socket);
        close_socket(socket);

        std::cout<<response<<std::endl;

        int pos=response.find("\r\n\r\n");

        if (pos == std::string::npos) {
        std::cerr << "[main] bad tracker response\n";
        return 1;
    }
        std::string body = response.substr(pos+4);

        TrackerResponse ResponseFromTracker;
        size_t dumpos=0;
        parse_response_dict(body,dumpos,ResponseFromTracker);
        vector<std::string>PeerList=ResponseFromTracker.peers;

        print_response(ResponseFromTracker);

        int listening_socket=create_server_socket(PORT);

        // Creating Thread List //

        std::vector<std::thread> all_threads;
        std::mutex threads_mutex;

        //created the struct needed for handshake (used by both seeder and leecher )

        handshake h;
        info_hashadder(h,t);
        peer_idadder(h,peer);

        // Call the Listener program in one thread //
        //Listener is used by both the seeder and leecher//
        //.....//


    if (command == "leecher") {

        if(PeerList.size()==0) {
            std::cerr << "[main] no peers from tracker"<<std::endl;
        }
        
        for(auto peerip: PeerList){
            int sep=peerip.find(':');
            if(sep==std::string::npos) continue;
            std::string ip   = peerip.substr(0, sep);
            int         port = std::stoi(peerip.substr(sep + 1));
            if (ip.empty() || port <= 0) continue;
            int fd=connect_to(ip,port);
            if (fd < 0) {
                std::cerr << "[main] cannot connect to " << ip << "\n";
                continue;
            }
            std::cout << "[main] connected to " << ip << ":" << port << std::endl;
            std::lock_guard<std::mutex> lock(threads_mutex);
        }

        // std::string peer0Ip=peer0IpAndPort.substr(0,colonPos);
        // std::cout<<peer0Ip<<std::endl;
        // int peer0Port=std::stoi(peer0IpAndPort.substr(colonPos+1));

        // std::string neighbourId=requestpeer(h,peer0Ip,peer0Port);

        // std::cout<<neighbourId<<std::endl;
        
    }

    else if(command=="seeder"){

    }

    return 0;
};