#include "../Parser/decoder.hpp"
#include "requestor.hpp"
#include<iostream>
#include<string>
#include<iomanip>


std::string url_encode(const std::string& raw) {
    std::string result;
    for (unsigned char c : raw) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%%%02x", c);
        result += buf;
    }
    return result;
}

std::string req_url(std::string& path,std::string &peer_id, Torrent&t, std::string host){

     std::string query = path + "?"
        + "info_hash=" + url_encode(t.info_raw)
        + "&peer_id="  + peer_id
        + "&port=6881"
        + "&uploaded=0"
        + "&downloaded=0"
        + "&left="     + std::to_string(t.length)
        + "&compact=1";

    std::string request =
        "GET " + query + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Connection: close\r\n"
        "\r\n";  

    return request;

    
}