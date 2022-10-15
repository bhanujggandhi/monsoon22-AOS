#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
// #include <openssl/sha.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 4
#define SERVERPORT 8081

using namespace std;

struct User {
    string userid;
    bool loggedin;
    string address;
};

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
queue<int*> thread_queue;
pair<string, int> connection_info = {"127.0.0.1", 8084};
User currUser;

void err(const char* msg);
void check(int status, string msg);
void splitutility(string str, char del, vector<string>& pth);
void* server_function(void* arg);
void connecttotracker(const char* filepath);
long getfilesize(string filename);
// string generateSHA(string filepath, long offset);
void client_function(const char* request, int CLIENTPORT);
void* start_thread(void* arg);
void* handle_connection(void* socket);

int main(int argc, char* argv[]) {
    string str("127.0.0.1:2001");
    currUser.address = str;
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, (void*)str.c_str());

    // connecttotracker("tracker_info.txt");
    while (1) {
        char request[255];
        memset(request, 0, 255);
        fflush(stdin);
        fgets(request, 255, stdin);
        vector<string> parsedReq;
        string req(request);
        int CLIENTPORT = connection_info.second;
        // int CLIENTPORT = 8081;
        if (CLIENTPORT == 0) continue;
        client_function(req.c_str(), CLIENTPORT);
    }
}

void err(const char* msg) { printf("%s\n", msg); }

void check(int status, string msg) {
    if (status < 0) {
        err(msg.c_str());
    }
}

void splitutility(string str, char del, vector<string>& arr) {
    string temp = "";

    for (int i = 0; i < str.size(); i++) {
        if (str[i] != del)
            temp += str[i];
        else {
            arr.push_back(temp);
            temp = "";
        }
    }

    arr.push_back(temp);
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

    // close(server_socket);
    shutdown(server_socket, SHUT_RDWR);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
}

void connecttotracker(const char* filepath) {
    char resolvedpath[_POSIX_PATH_MAX];
    if (realpath(filepath, resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        return;
    }

    std::ifstream file(resolvedpath);
    if (file.is_open()) {
        std::string line;
        // int i = 1;
        while (std::getline(file, line)) {
            vector<string> ipport;
            splitutility(line, ':', ipport);

            int portno = stoi(ipport[1]);
            int server_socket;
            if (server_socket = socket(AF_INET, SOCK_STREAM, 0) < 0) {
                printf("Error: Opening socket\n");
                exit(1);
            }

            struct sockaddr_in server_address;
            bzero((char*)&server_address, sizeof(server_address));

            server_address.sin_family = AF_INET;

            server_address.sin_addr.s_addr = INADDR_ANY;

            server_address.sin_port = htons(portno);

            if (connect(server_socket, (struct sockaddr*)&server_address,
                        sizeof(server_address)) < 0) {
                printf("Could not connect\n");
                continue;
            }
            connection_info.first = ipport[0];
            connection_info.second = stoi(ipport[1]);
            printf("Connected successfully\n");
            file.close();
            close(server_socket);
            return;
        }

        file.close();
        printf("No tracker online\n");
        exit(1);
    }
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

    vector<string> reqarr;
    string req(request);
    if (req.length() > 1) {
        req.pop_back();
    }
    splitutility(req, ' ', reqarr);

    char buffer[BUFSIZ];
    memset(buffer, 0, BUFSIZ);
    if (reqarr[0] == "create_user") {
        if (reqarr.size() != 3) {
            string msg =
                "1:Invalid Number of arguments for Login\nUSAGE - create_user "
                "<user_id> <password>\n";
            printf("%s", msg.c_str());
            return;
        }

        req += " " + currUser.address + "\n";
        int n = write(server_socket, req.c_str(), req.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);
        if (resarr[0] == "2") {
            currUser.userid = resarr[1];
            currUser.loggedin = true;
            printf("%s\n", resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "login") {
        if (reqarr.size() != 3) {
            string msg =
                "1:Invalid Number of arguments for Login\nUSAGE - login "
                "<user_id> <password>\n";
            printf("%s", msg.c_str());
            return;
        }
        if (currUser.loggedin) {
            printf("You are already logged in as %s\nPlease logout first!\n",
                   currUser.userid.c_str());
            return;
        }
        req += " " + currUser.address + "\n";
        int n = write(server_socket, req.c_str(), req.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            currUser.userid = resarr[1];
            currUser.loggedin = true;
            printf("%s\n", resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "create_group") {
        if (reqarr.size() < 2) {
            printf("Invalid number of arguments\n");
            printf("USAGE: create_group <group_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }
        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("%s\n", resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "join_group") {
        if (reqarr.size() < 2) {
            printf("Invalid number of arguments\n");
            printf("USAGE: join_group <group_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "leave_group") {
        if (reqarr.size() < 2) {
            printf("Invalid number of arguments\n");
            printf("USAGE: join_group <group_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "list_requests") {
        if (reqarr.size() < 2) {
            printf("Invalid number of arguments\n");
            printf("USAGE: list_requests <group_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "accept_request") {
        if (reqarr.size() != 3) {
            printf("Invalid number of arguments\n");
            printf("USAGE: accept_request <group_id> <user_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (req == "list_groups") {
        if (reqarr.size() != 1) {
            printf("Invalid number of arguments\n");
            printf("USAGE: list_groups");
            return;
        }
        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "list_files") {
        if (reqarr.size() != 2) {
            printf("Invalid number of arguments\n");
            printf("USAGE: list_files <group_id>");
            return;
        }
        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "upload_file") {
        if (reqarr.size() != 3) {
            printf("Invalid number of arguments\n");
            printf("USAGE: upload_file <file_path> <group_id>");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        // string sha = generateSHA(reqarr[1], 0);
        string sha = "hello";
        long filesize = getfilesize(reqarr[1]);
        string groupid = reqarr[2];

        char resolvedpath[_POSIX_PATH_MAX];

        if (realpath(reqarr[1].c_str(), resolvedpath) == NULL) {
            printf("ERROR: bad path %s\n", resolvedpath);
            return;
        }

        string resp(resolvedpath);

        string preq = reqarr[0] + " " + resp + " " + reqarr[2] + " " + sha +
                      " " + to_string(filesize) + " " + currUser.userid + " " +
                      currUser.address + "\n";

        printf("%s\n", preq.c_str());

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "2") {
            printf("[%s]: %s\n", resarr[1].c_str(), resarr[2].c_str());
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (req == "logout") {
        if (currUser.loggedin == false) {
            printf("You are not logged in!\n");

        } else {
            req = req + " " + currUser.userid + "\n";
            printf("Sending req %s", req.c_str());
            char charreq[req.length() + 1];
            strcpy(charreq, req.c_str());
            charreq[req.length() + 1] = 0;
            int n = send(server_socket, charreq, strlen(charreq), 0);
            if (n < 0) err("ERROR: writing to socket");

            size_t size;
            if (read(server_socket, buffer, BUFSIZ) < 0) {
                printf("Couldn't get response from the tracker\n");
                return;
            }

            vector<string> resarr;
            string res(buffer);
            splitutility(res, ':', resarr);

            if (resarr[0] == "2") {
                currUser.userid = resarr[1];
                currUser.loggedin = false;
                printf("%s\n", resarr[2].c_str());
            } else if (resarr[0] == "1") {
                printf("%s\n", resarr[1].c_str());
            }
        }
    } else {
        printf("Invalid command\nTry again!\n");
        return;
    }

    // printf("%s\n", currUser.userid.c_str());

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

void* handle_connection(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    char request[_POSIX_PATH_MAX];
    bzero(request, _POSIX_PATH_MAX);

    size_t bytes_read;
    int msgsize = 0;

    while ((bytes_read = read(client_socket, request + msgsize,
                              sizeof(request) - msgsize - 1)) > 0) {
        msgsize += bytes_read;
        if (msgsize > BUFSIZ - 1 or request[msgsize - 1] == '\n') break;
    }
    check(bytes_read, "recv error");
    request[msgsize - 1] = 0;

    char resolvedpath[_POSIX_PATH_MAX];

    fflush(stdout);
    if (realpath(request, resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        close(client_socket);
        return NULL;
    }

    char buffer[BUFSIZ];

    FILE* fp = fopen(resolvedpath, "r");
    if (fp == NULL) {
        printf("ERROR(open) %s\n", buffer);
        close(client_socket);
        return NULL;
    }

    while ((bytes_read = fread(buffer, 1, BUFSIZ, fp)) > 0) {
        printf("Sending %ld bytes\n", bytes_read);
        write(client_socket, buffer, bytes_read);
    }

    close(client_socket);
    fclose(fp);
    return NULL;
}

void recieved_file(string path, int server_socket) {
    /* Take source destination from the args and realpath */
    char buff[BUFSIZ];
    bzero(buff, BUFSIZ);

    int d = open(path.c_str(), O_WRONLY | O_CREAT,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (d == -1) {
        err("Destination file cannot be opened");
        close(d);
        return;
    }

    size_t size;
    while ((size = read(server_socket, buff, BUFSIZ)) > 0) {
        // sleep(1);
        printf("Got %ld bytes\n", size);
        write(d, buff, size);
    }

    close(d);

    printf("File Recieved Succesfully!\n");
}

// string generateSHA(string filepath, long offset) {
//     char resolvedpath[_POSIX_PATH_MAX];
//     if (realpath(filepath.c_str(), resolvedpath) == NULL) {
//         printf("ERR: bad path %s\n", resolvedpath);
//         return NULL;
//     }

//     FILE* fd = fopen(resolvedpath, "rb");
//     char shabuf[SHA_DIGEST_LENGTH];
//     bzero(shabuf, sizeof(shabuf));
//     fseek(fd, offset, SEEK_SET);
//     fread(shabuf, 1, SHA_DIGEST_LENGTH, fd);

//     unsigned char SHA_Buffer[SHA_DIGEST_LENGTH];
//     char buffer[SHA_DIGEST_LENGTH * 2];
//     int i;
//     bzero(buffer, sizeof(buffer));
//     bzero(SHA_Buffer, sizeof(SHA_Buffer));
//     SHA1((unsigned char*)shabuf, 20, SHA_Buffer);

//     for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
//         sprintf((char*)&(buffer[i * 2]), "%02x", SHA_Buffer[i]);
//     }

//     fclose(fd);
//     string shastr(buffer);
//     return shastr;
// }

long getfilesize(string filename) {
    struct stat sbuf;
    int f = stat(filename.c_str(), &sbuf);
    return f == 0 ? sbuf.st_size : -1;
}