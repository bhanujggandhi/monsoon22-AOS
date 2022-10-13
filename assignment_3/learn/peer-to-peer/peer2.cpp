#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <queue>
#include <string>

#define THREAD_POOL_SIZE 4
#define SERVERPORT 8082

using namespace std;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
queue<int*> thread_queue;

void error(const string msg);
void check(int status, string msg);
void* server_function(void* arg);
void client_function(char* request, int CLIENTPORT);
void* start_thread(void* arg);
void* handle_connection(void* socket);

int main(int argc, char* argv[]) {
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, NULL);

    while (1) {
        char portreq[5];
        char request[255];
        memset(portreq, 0, 5);
        memset(request, 0, 255);
        fgets(portreq, 5, stdin);
        fflush(stdin);
        fgets(request, 255, stdin);
        int CLIENTPORT = atoi(portreq);
        printf("%d incoming port\n", CLIENTPORT);
        client_function(request, CLIENTPORT);
    }
}

void err(const char* msg) {
    perror(msg);
    exit(1);
}

void check(int status, string msg) {
    if (status < 0) {
        err(msg.c_str());
    }
}

void* server_function(void* arg) {
    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Socket Cannot be Opened!");

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));

    int portno = SERVERPORT;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);

    check(bind(server_socket, (struct sockaddr*)&server_addr,
               sizeof(server_addr)),
          "ERROR: on binding");

    check(listen(server_socket, 100), "ERROR: Listen Failed");

    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);

    pthread_t th[THREAD_POOL_SIZE];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        check(pthread_create(&th[i], NULL, start_thread, NULL),
              "Failed to create the thread");
    }

    while (1) {
        printf("Waiting for connections...\n");

        int client_socket;
        check(client_socket = accept(server_socket,
                                     (struct sockaddr*)&client_addr, &clilen),
              "ERROR: on Accept");

        printf("Server: Got connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int* pclient = (int*)malloc(sizeof(int));
        *pclient = client_socket;

        pthread_mutex_lock(&mutexQueue);
        thread_queue.push(pclient);
        pthread_cond_signal(&condQueue);
        pthread_mutex_unlock(&mutexQueue);
    }

    close(server_socket);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
}

void client_function(char* request, int CLIENTPORT) {
    int portno = CLIENTPORT;
    int client_socket;
    check(client_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Opening PORT");

    struct sockaddr_in server_address;
    bzero((char*)&server_address, sizeof(server_address));

    server_address.sin_family = AF_INET;

    server_address.sin_addr.s_addr = INADDR_ANY;
    // OR
    // struct hostent* server = gethostbyname("phoenix");
    // if (server == NULL) {
    //     error("ERROR: No such host");
    // }
    // bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr,
    //       server->h_length);

    server_address.sin_port = htons(portno);

    if (connect(client_socket, (struct sockaddr*)&server_address,
                sizeof(server_address)) < 0)
        err("Error Connecting");

    int n = write(client_socket, request, strlen(request));
    if (n < 0) err("ERROR: writing to socket");
}

void* start_thread(void* arg) {
    while (true) {
        int* pclient;
        pthread_mutex_lock(&mutexQueue);
        if (thread_queue.empty()) {
            pclient = NULL;
            pthread_cond_wait(&condQueue, &mutexQueue);
            if (!thread_queue.empty()) {
                pclient = thread_queue.front();
                thread_queue.pop();
            }
        } else {
            pclient = thread_queue.front();
            thread_queue.pop();
        }
        pthread_mutex_unlock(&mutexQueue);

        if (pclient != NULL) {
            handle_connection(pclient);
        }
    }
    return NULL;
}

void* handle_connection(void* arg) {
    int client_socket = *(int*)arg;
    char request[255];
    bzero(request, 255);
    read(client_socket, request, 255);
    printf("REQUEST: %s\n", request);
    close(client_socket);
    return NULL;
}