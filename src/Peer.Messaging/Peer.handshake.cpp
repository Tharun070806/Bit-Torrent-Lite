#include "Peer.handshake.hpp"
#include <iostream>
#include <string>

void info_hashadder(handshake&h, Torrent &t){
    h.info_hash=t.info_raw;
}

void peer_idadder(handshake&h, std::string &peerid){
    h.peer_id=peerid;
}