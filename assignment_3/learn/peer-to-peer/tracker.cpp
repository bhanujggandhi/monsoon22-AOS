#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 4
#define SERVERPORT 8080

using namespace std;

// Globals
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
queue<int*> thread_queue;
unordered_map<string, string> user_map;
unordered_map<string, bool> loggedin_map;

// Functions
void err(const char* msg);
void check(int status, string msg);
void splitutility(string str, char del, vector<string>& pth);
void* server_function(void* arg);
void client_function(const char* request, int CLIENTPORT);
void* start_thread(void* arg);
void download_file(char* path, int client_socket);
void* handle_connection(void* socket);

int main(int argc, char* argv[]) {
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, (void*)argv[1]);

    while (1) {
        char request[255];
        memset(request, 0, 255);
        fflush(stdin);
        fgets(request, 255, stdin);
        vector<string> parsedReq;
        string req(request);
        splitutility(req, ':', parsedReq);
        int CLIENTPORT = atoi(parsedReq[0].c_str());
        // int CLIENTPORT = 8081;
        if (CLIENTPORT == 0) continue;
        client_function(parsedReq[1].c_str(), CLIENTPORT);
    }
}

void err(const char* msg) { printf("%s\n", msg); }

void check(int status, string msg) {
    if (status < 0) {
        err(msg.c_str());
    }
}

void splitutility(string str, char del, vector<string>& pth) {
    string temp = "";

    for (int i = 0; i < str.size(); i++) {
        if (str[i] != del)
            temp += str[i];
        else {
            pth.push_back(temp);
            temp = "";
        }
    }

    pth.push_back(temp);
}

void* server_function(void* arg) {
    // Parse IP:PORT
    char* ipport = (char*)arg;
    vector<string> ipportsplit;
    string ipportstr(ipport);
    splitutility(ipportstr, ':', ipportsplit);

    int portno = stoi(ipportsplit[1]);

    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Socket Cannot be Opened!");

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));

    // int portno = SERVERPORT;
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ipportsplit[0].c_str(), &server_addr.sin_addr) ==
        0) {
        perror("Invalid IP");
        exit(1);
    }
    // server_addr.sin_addr.s_addr = INADDR_ANY;
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

    // close(server_socket);
    shutdown(server_socket, SHUT_RDWR);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
}

void client_function(const char* request, int CLIENTPORT) {
    int portno = CLIENTPORT;
    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
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

    if (connect(server_socket, (struct sockaddr*)&server_address,
                sizeof(server_address)) < 0) {
        err("Error Connecting");
        return;
    }

    int n = write(server_socket, request, strlen(request));
    if (n < 0) err("ERROR: writing to socket");

    char buff[BUFSIZ];
    bzero(buff, BUFSIZ);

    /* Take source destination from the args and realpath */

    int d = open("copied.txt", O_WRONLY | O_CREAT,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (d == -1) {
        err("Destination file cannot be opened");
        close(d);
        return;
    }

    size_t size;
    while ((size = read(server_socket, buff, BUFSIZ)) > 0) {
        sleep(2);
        write(d, buff, size);
    }

    close(d);

    printf("File Transferred Succesfully!\n");
    close(server_socket);
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

void download_file(char* path, int client_socket) {
    char resolvedpath[_POSIX_PATH_MAX];
    size_t bytes_read;

    fflush(stdout);
    if (realpath(path, resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        close(client_socket);
        return;
    }

    char buffer[BUFSIZ];

    FILE* fp = fopen(resolvedpath, "r");
    if (fp == NULL) {
        printf("ERROR(open) %s\n", buffer);
        close(client_socket);
        return;
    }

    while ((bytes_read = fread(buffer, 1, BUFSIZ, fp)) > 0) {
        printf("Sending %ld bytes\n", bytes_read);
        write(client_socket, buffer, bytes_read);
    }

    close(client_socket);
    fclose(fp);
    return;
}

void* handle_connection(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    char request[_POSIX_PATH_MAX];
    bzero(request, _POSIX_PATH_MAX);

    size_t bytes_read;
    int req_size = 0;

    while ((bytes_read = read(client_socket, request + req_size,
                              sizeof(request) - req_size - 1)) > 0) {
        req_size += bytes_read;
        if (req_size > _POSIX_PATH_MAX - 1 or request[req_size - 1] == '\n')
            break;
    }
    check(bytes_read, "recv error");
    request[req_size - 1] = 0;

    vector<string> reqarr;
    string req(request);
    splitutility(req, ' ', reqarr);

    if (reqarr[0] == "create_user") {
        if (reqarr.size() > 3) {
            string msg =
                "1:Invalid Number of arguments for Create User\nUSAGE - "
                "create_user <user_id> <password>\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        string userid = reqarr[1];
        string password = reqarr[2];

        if (user_map.find(userid) != user_map.end()) {
            string msg =
                "1:User ID already exists\nEither Login or create an account "
                "with unique User ID\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        user_map.insert({userid, password});
        loggedin_map[userid] = true;
        string res = "1:" + userid + ":User created successfully!\n";
        write(client_socket, res.c_str(), res.size());
    } else if (reqarr[0] == "login") {
        if (reqarr.size() > 3) {
            string msg =
                "1:Invalid Number of arguments for Login\nUSAGE - login "
                "<user_id> <password>\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        string userid = reqarr[1];
        string password = reqarr[2];

        if (user_map.find(userid) == user_map.end()) {
            string msg =
                "1:User ID does not exist\nPlease create an account to "
                "continue\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (password == user_map[userid]) {
            loggedin_map[userid] = true;
            string res = "2:" + userid + ":User logged in successfully!\n";
            write(client_socket, res.c_str(), res.size());
            return NULL;
        }

        loggedin_map[userid] = false;
        string msg = "1:Password is incorrect, please try again\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "logout") {
        if (loggedin_map.find(reqarr[1]) == loggedin_map.end()) {
            string msg = "1:User is not logged in\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        loggedin_map[reqarr[1]] = false;
        string msg = "1:Logged out successfully!\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else {
        write(client_socket, "0:Please enter a valid command\n", 29);
        return NULL;
    }

    return NULL;
}