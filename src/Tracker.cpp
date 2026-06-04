#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include "./utils/socket_utils.hpp"

using namespace std;


unordered_map<string, string> parse_query(const string& query) {
    unordered_map<string, string> params;

    size_t start = 0;
    while (start < query.size()) {
        size_t eq = query.find('=', start);
        size_t amp = query.find('&', start);

        if (eq == string::npos) break;

        string key = query.substr(start, eq - start);
        string val = query.substr(eq + 1,
                      (amp == string::npos ? query.size() : amp) - eq - 1);

        params[key] = val;

        if (amp == string::npos) break;
        start = amp + 1;
    }

    return params;
}


string build_tracker_response() {
    string peers;

 
    peers.push_back(127);
    peers.push_back(0);
    peers.push_back(0);
    peers.push_back(1);

    uint16_t port = htons(6881);
    peers.push_back((port >> 8) & 0xFF);
    peers.push_back(port & 0xFF);

    // bencode
    string body =
        "d"
        "8:completei4e"
        "10:incompletei0e"
        "8:intervali60e"
        "12:min intervali60e"
        "5:peers" + to_string(peers.size()) + ":" + peers +
        "e";

    return body;
}

void handle_client(int client_fd) {
    char buffer[4096];
    string request;

    recv_inf(request, buffer, client_fd);

    cout << "Incoming request:\n" << request << endl;

    // parse first line
    stringstream ss(request);
    string method, full_path, version;
    ss >> method >> full_path >> version;

    size_t qpos = full_path.find('?');

    string path = full_path.substr(0, qpos);
    string query = (qpos != string::npos) ? full_path.substr(qpos + 1) : "";

    auto params = parse_query(query);

   
    for (auto& [k, v] : params) {
        cout << k << " = " << v << endl;
    }

    
    string body = build_tracker_response();

    string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    send_all(client_fd, response.c_str(), response.size());

    close_socket(client_fd);
}


int main() {
    int server_fd = create_server_socket(8080);

    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        handle_client(client_fd);
    }

    close_socket(server_fd);
    return 0;
}
