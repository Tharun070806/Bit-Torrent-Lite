#pragma once
#include<iostream>
#include<string>
#include "../Parser/decoder.hpp"

struct handshake{
    char byte19 = 19;
    std::string protocol="BitTorrent protocol";
    std::string zero=std::string(8,'\0');
    std::string info_hash;
    std::string peer_id;
};

void info_hashadder(handshake&h, Torrent &t);
void peer_idadder(handshake&h, std::string &peerid);
