#include "peer_handshake.hpp"


void info_hashadder(handshake&h, Torrent &t){
    h.info_hash=sha1_withouthex(t.info_raw);
}

void peer_idadder(handshake&h, std::string &peerid){
    h.peer_id=peerid;
}

std::string requestpeer(handshake &h, std::string peerip, int peerport){
    
    //Constructing peer message 

    unsigned char message[69];
    unsigned char response [69];
    int ptr=0;
    message[ptr++]=h.byte19;
    for(int i=ptr;i<ptr+19;i++){
        message[i]=h.protocol[i-ptr];
    }
    ptr=20;
    for(int i=ptr;i<ptr+8;i++){
        message[i]=h.zero[i-ptr];
    }
    ptr=28;
    for(int i=ptr;i<ptr+20;i++){
        message[i]=h.info_hash[i-ptr];
    }
    ptr=48;
    for(int i=ptr;i<ptr+20;i++){
        message[i]=h.peer_id[i-ptr];
    }

    //sending peer message 

    int socket=connect_to(peerip, peerport);
    bool sent=send_all(socket,message,68);
    bool rcv=recv_exact(socket,response, 68 );

    std::string peerid;
    for(int i=48;i<68;i++){
        peerid.push_back(response[i]);
    }

    std::string peerIdHexForm=hexconverter(peerid);
    return peerIdHexForm;
}