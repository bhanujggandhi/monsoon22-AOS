#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
using namespace std;
#define PORT 8080

void rx(int new_socket) {
    int bytes_received, loop = 5;
    char buffer[1024] = {0};
    while (loop--) {
        bytes_received = recv(new_socket, buffer, 1024, 0);
        cout << "The messege from client is: " << buffer << endl;
        memset(buffer, 0, 1024);
    }
}

void tx(int new_socket) {
    int loop = 5;
    string hello;
    while (loop--) {
        getline(cin, hello);
        send(new_socket, hello.c_str(), strlen(hello.c_str()), 0);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,
                             (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    thread t_rx(rx, new_socket);
    thread t_tx(tx, new_socket);
    t_tx.join();
    t_rx.join();
    /*while(!t_rx.joinable() || !t_tx.joinable()){
            if(t_tx.joinable())
    if(t_rx.joinable())
    }*/
    fflush(stdin);
    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}