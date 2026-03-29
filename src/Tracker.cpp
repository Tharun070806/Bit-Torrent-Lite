#include<stdio.h>
#include<stdlib.h>
#include "socket_utils.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include<arpa/inet.h>
#include <cstring>
using namespace std;

#define PORT 9000


class parser{
    public:
        string request;
        string port;
        string method;
        string info_hash;

        parser(string &request){
            this->request=request;
        }

        string find_method(){
            if(request.find("GET /announce"))
        }
};


int main(){
    int server = create_server_socket(PORT);
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int client = accept(server, (sockaddr*)&client_addr, &len);
    string request;
    receving_header(server, request);

}