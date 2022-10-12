#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

using namespace std;

void error(const string msg);
void check(int status, string msg);
void* server_function(void* arg);
void client_function(char* request);

int main() {
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, NULL);

    while (1) {
        char request[255];
        memset(request, 0, 255);
        fgets(request, 255, stdin);
        client_function(request);
    }
}

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void check(int status, string msg) {
    if (status < 0) {
        error(msg.c_str());
    }
}

void* server_function(void* arg) {
    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Socket Cannot be Opened!");

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));

    int portno = 8080;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);

    check(bind(server_socket, (struct sockaddr*)&server_addr,
               sizeof(server_addr)),
          "ERROR: on binding");

    check(listen(server_socket, 100), "ERROR: Listen Failed");

    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);

    while (1) {
        printf("Waiting for connections...\n");

        int client_socket;
        check(client_socket = accept(server_socket,
                                     (struct sockaddr*)&client_addr, &clilen),
              "ERROR: on Accept");

        printf("Server: Got connection from %s port %d on the socket number\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char request[255];
        bzero(request, 255);
        read(client_socket, request, 255);
        printf("REQUEST: %s\n", request);
        close(client_socket);
    }
}

void client_function(char* request) {
    int portno = 8080;
    int client_socket;
    check(client_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Opening PORT");

    struct sockaddr_in server_address;
    bzero((char*)&server_address, sizeof(server_address));

    server_address.sin_family = AF_INET;

    // server_address.sin_addr.s_addr = INADDR_ANY;
    // OR
    struct hostent* server = gethostbyname("phoenix");
    if (server == NULL) {
        error("ERROR: No such host");
    }
    bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr,
          server->h_length);

    server_address.sin_port = htons(portno);

    if (connect(client_socket, (struct sockaddr*)&server_address,
                sizeof(server_address)) < 0)
        error("Error Connecting");

    int n = write(client_socket, request, strlen(request));
    if (n < 0) error("ERROR: writing to socket");
}