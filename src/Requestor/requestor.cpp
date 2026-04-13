
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

std::string req_url(std::string& path,std::string &peer_id, Torrent&t, std::string host, int tracker_port, bool leecher, int port){

     std::string query = path + "?"
        + "info_hash=" + url_encode(sha1_withouthex(t.info_raw))
        + "&peer_id="  + peer_id
        + "&port="+(std::to_string(port))
        + "&uploaded=0"
        + "&downloaded=0"
        + "&left="     + ((leecher==1) ? std::to_string(t.length): "0")
        + "&compact=1";

    if(!leecher){
        query+="&event=completed";
    }

    std::string request =
    "GET " + query + " HTTP/1.1\r\n"
    "Host: " + host + (tracker_port == 80 ? "" : ":" + std::to_string(tracker_port)) + "\r\n"
    "Connection: close\r\n"
    "\r\n";

    return request;

    
}