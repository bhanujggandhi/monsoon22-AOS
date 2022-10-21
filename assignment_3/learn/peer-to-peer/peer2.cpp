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
#define DOWNLOAD_THREAD_POOL 20
#define SERVERPORT 8082
#define CHUNKSIZE 524288

using namespace std;

struct User {
    string userid;
    bool loggedin;
    string address;
};

struct FileStr {
    string filepath;
    string SHA;
    long filesize;
    long chunks = ceil(filesize / (long)BUFSIZ);
    vector<bool> chunkpresent;
    vector<pair<int, string>> chunksha;
};

struct DownloadData {
    int chunknumber;
    string srcfilepath;
    string destfilepath;
    vector<string> peers;
    string filesha;

    DownloadData() {}
};

struct DownloadPassThread {
    string destinationpath;
    unordered_map<long, vector<string>> chunktomap;
    string srcfilepath;
    string filesha;
};

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
queue<int*> thread_queue;

pthread_mutex_t mutexDownQueue;
pthread_cond_t condDownQueue;
queue<DownloadData*> threadDownQueue;

pair<string, int> connection_info = {"127.0.0.1", 8084};
User currUser;
unordered_map<string, FileStr*> filetomap;

void err(const char* msg);
void check(int status, string msg);
bool cmp(DownloadData* p1, DownloadData* p2);
void splitutility(string str, char del, vector<string>& pth);
void* server_function(void* arg);
void connecttotracker(const char* filepath);
long getfilesize(string filename);
// string generateSHA(string filepath, long offset);
void userschunkmapinfo(unordered_map<long, vector<string>>& chunktomap,
                       vector<string>& clientarr, string& filepath);
void* downloadexec(void* arg);
void client_function(const char* request, int CLIENTPORT);
void* start_thread(void* arg);
void* start_down_thread(void* arg);
void* handle_connection(void* socket);
void* downloadstart(void* donwloadtransfer);

int main(int argc, char* argv[]) {
    string str("127.0.0.1:2002");
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

    // currUser.loggedin = true;
    // currUser.address = "127.0.0.1:2001";
    // currUser.userid = "bhanuj";

    // FileStr* fl = new FileStr();
    // fl->filepath =
    //     "/mnt/LINUXDATA/bhanujggandhi/Learning/iiit/sem1/aos/assignment_3/"
    //     "temp.pdf";
    // fl->filesize = getfilesize(
    //     "/mnt/LINUXDATA/bhanujggandhi/Learning/iiit/sem1/aos/assignment_3/"
    //     "temp.pdf");
    // fl->chunks = ceil((double)fl->filesize / CHUNKSIZE);
    // vector<bool> c(fl->chunks, true);
    // fl->chunkpresent = c;

    // filetomap.insert({fl->filepath, fl});

    // server_function((void*)str.c_str());
}

void err(const char* msg) { printf("%s\n", msg); }

void check(int status, string msg) {
    if (status < 0) {
        err(msg.c_str());
    }
}

bool cmp(DownloadData* p1, DownloadData* p2) {
    return p1->peers.size() < p2->peers.size();
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
            printf("USAGE: create_group <group_id>\n");
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
            printf("USAGE: join_group <group_id>\n");
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
            printf("USAGE: join_group <group_id>\n");
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
            printf("USAGE: list_requests <group_id>\n");
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
            printf("USAGE: accept_request <group_id> <user_id>\n");
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
            printf("USAGE: list_groups\n");
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
            printf("USAGE: list_files <group_id>\n");
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
            printf("USAGE: upload_file <file_path> <group_id>\n");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }
        string sha = "hello";
        long filesize = getfilesize(reqarr[1]);
        string groupid = reqarr[2];

        char resolvedpath[_POSIX_PATH_MAX];

        if (realpath(reqarr[1].c_str(), resolvedpath) == NULL) {
            printf("ERROR: bad path %s\n", resolvedpath);
            return;
        }

        string resp(resolvedpath);

        FileStr* currFile = new FileStr();
        currFile->filepath = resolvedpath;
        currFile->filesize = filesize;
        currFile->chunks = ceil((double)filesize / CHUNKSIZE);
        vector<bool> tempchunkpresent(ceil((double)filesize / CHUNKSIZE));

        string concatenatedSHA = "";
        long off = 0;
        long flsz = filesize;
        while (flsz > 0) {
            // string currsha = generateSHA(resolvedpath, off);
            string currsha = "hello" + to_string(flsz);
            currFile->chunksha.push_back(
                {ceil((double)off / CHUNKSIZE), currsha});
            tempchunkpresent[ceil((double)off / CHUNKSIZE)] = true;
            concatenatedSHA += currsha;
            flsz -= CHUNKSIZE;
            off += CHUNKSIZE;
        }

        concatenatedSHA =
            "68b41f1e817a0f2c20693c5aba6d8aab48919e652a300385b1e023f2d25ab97842"
            "1799552b6fd8e71b1dd956a417ce614d934a4c023da7a1ebff335634cc197ac68e"
            "dc7292bb843b77f29264879d147114f5d0dec6422e21f811604ce4b93d984f5187"
            "76735eda98ca450ed9a8bddefc92aa273d06ada97e44ab1e080c08857d3f8d4fa5"
            "a01d5076dbc32a86f5a8e4d2fdcc382cf13c190e7f8a2a2e90ef0abf9730f8a36e"
            "cd68bc35947f67cf649771002712b3bfb195210869dbe9b807a395341c46ca07fd"
            "99a9";
        currFile->chunkpresent = tempchunkpresent;
        currFile->SHA = concatenatedSHA;

        string preq = reqarr[0] + " " + resp + " " + reqarr[2] + " " +
                      concatenatedSHA + " " + to_string(filesize) + " " +
                      currUser.userid + "\n";

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
            filetomap.insert({resolvedpath, currFile});
        } else if (resarr[0] == "1") {
            printf("%s\n", resarr[1].c_str());
        }
    } else if (reqarr[0] == "download_file") {
        if (reqarr.size() != 4) {
            printf("Invalid usage of command\n");
            printf(
                "USAGE: download_file <group_id> <file_name> "
                "<destination_path>\n");
            return;
        }

        if (!currUser.loggedin) {
            printf("You are not logged! Please login\n");
            return;
        }

        char resolvedpath[_POSIX_PATH_MAX];
        if (realpath(reqarr[2].c_str(), resolvedpath) == NULL) {
            printf("ERROR: bad path %s\n", resolvedpath);
            return;
        }

        string resp(resolvedpath);
        string preq = req + " " + currUser.userid + "\n";

        int n = write(server_socket, preq.c_str(), preq.size());
        if (n < 0) err("ERROR: writing to socket");

        size_t size;
        if (read(server_socket, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }
        printf("%s\n", buffer);

        if (buffer[0] == 'S') {
            vector<string> clientarr;
            string res(buffer);
            splitutility(res.substr(2), ' ', clientarr);
            string fullfilesha = clientarr[0];
            vector<string> clients;
            for (int i = 1; i < clientarr.size(); i++) {
                clients.push_back(clientarr[i]);
            }

            unordered_map<long, vector<string>> chunktomap;

            userschunkmapinfo(chunktomap, clients, resp);

            DownloadPassThread* transferdata = new DownloadPassThread();
            transferdata->chunktomap = chunktomap;
            transferdata->destinationpath = reqarr[3];
            transferdata->filesha = fullfilesha;
            transferdata->srcfilepath = resolvedpath;

            pthread_t download_thread;
            pthread_create(&download_thread, NULL, downloadstart, transferdata);

        } else if (buffer[0] == '1') {
            vector<string> resarr;
            string res(buffer);
            splitutility(res, ':', resarr);
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

void* start_down_thread(void* arg) {
    int breakcond = *(int*)arg;
    while (true) {
        DownloadData* pclient;
        pthread_mutex_lock(&mutexDownQueue);
        if (threadDownQueue.empty()) {
            pclient = NULL;
            pthread_cond_wait(&condDownQueue, &mutexDownQueue);
            if (!threadDownQueue.empty()) {
                pclient = threadDownQueue.front();
                threadDownQueue.pop();
            }
        } else {
            pclient = threadDownQueue.front();
            threadDownQueue.pop();
            breakcond--;
        }
        pthread_mutex_unlock(&mutexDownQueue);

        if (pclient != NULL) {
            downloadexec(pclient);
        }
        if (breakcond <= 0) {
            break;
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

    vector<string> reqarr;
    string req(request);
    splitutility(req, ' ', reqarr);
    if (reqarr.empty()) {
        string msg = "Something went wrong\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    }

    if (reqarr[0] == "getchunks") {

        if (filetomap.find(reqarr[1]) == filetomap.end()) {
            string res = "-1";
            write(client_socket, res.c_str(), res.size());
            return NULL;
        }

        auto currFile = filetomap[reqarr[1]];
        string res = "1:";

        for (long i = 0; i < currFile->chunkpresent.size(); i++) {
            res += to_string(i) + " ";
        }
        write(client_socket, res.c_str(), res.size());
        return NULL;
    } else if (reqarr[0] == "download") {
        string filepath = reqarr[1];
        int chunknumber = stoi(reqarr[2]);
        long totalfilesize = getfilesize(filepath) - (chunknumber * CHUNKSIZE);

        int fd = open(filepath.c_str(), O_RDONLY);
        off64_t offset = chunknumber * CHUNKSIZE;
        int filesize = CHUNKSIZE;
        size_t readbytes;
        char buff[BUFSIZ];
        while ((readbytes = pread64(fd, buff, BUFSIZ, offset)) > 0 &&
               filesize > 0 && totalfilesize > 0) {
            write(client_socket, buff, readbytes);
            offset += readbytes;
            bzero(buff, BUFSIZ);
            filesize -= readbytes;
            totalfilesize -= readbytes;
        }
        close(fd);
        return NULL;
    } else {
        string res = "1:Invalid command\n";
        write(client_socket, res.c_str(), res.size());
        return NULL;
    }
}

void userschunkmapinfo(unordered_map<long, vector<string>>& chunktomap,
                       vector<string>& clientarr, string& filepath) {
    for (int i = 0; i < clientarr.size(); i++) {
        vector<string> ipport;
        string curr = clientarr[i];
        splitutility(curr, ':', ipport);
        int port = stoi(ipport[1]);

        int peerfd = socket(AF_INET, SOCK_STREAM, 0);
        if (peerfd < 0) {
            printf("Error: In opening socket\n");
            return;
        }

        struct sockaddr_in peer_address;
        peer_address.sin_family = AF_INET;
        peer_address.sin_addr.s_addr = INADDR_ANY;
        peer_address.sin_port = htons(port);

        if (connect(peerfd, (struct sockaddr*)&peer_address,
                    sizeof(peer_address)) < 0) {
            printf("Error: In creating connection\n");
            return;
        }

        char buffer[BUFSIZ];
        bzero(buffer, BUFSIZ);

        string req = "getchunks ";
        req += filepath + "\n";
        cout << req << endl;

        int n = write(peerfd, req.c_str(), req.size());
        if (n < 0) {
            printf("Couldn't send the request\n");
            return;
        }

        if (read(peerfd, buffer, BUFSIZ) < 0) {
            printf("Couldn't get response from the tracker\n");
            return;
        }

        vector<string> resarr;
        printf("%s\n", buffer);
        string res(buffer);
        splitutility(res, ':', resarr);

        if (resarr[0] == "1") {
            vector<string> chunkdetails;
            splitutility(resarr[1], ' ', chunkdetails);

            for (auto x : chunkdetails) {
                if (x != "") {
                    if (chunktomap.find(stol(x)) == chunktomap.end()) {
                        chunktomap.insert({stol(x), {}});
                    }
                    chunktomap[stol(x)].push_back(curr);
                }
            }
        }
    }
}

void* downloadexec(void* arg) {
    DownloadData chunkinfo = *(DownloadData*)arg;

    vector<string> clientvec = chunkinfo.peers;
    string fullfilesha = chunkinfo.filesha;
    int noofclients = clientvec.size();
    string filepath = chunkinfo.srcfilepath;
    int chunknumber = chunkinfo.chunknumber;

    string curr = clientvec[rand() % noofclients];
    vector<string> ipport;
    splitutility(curr, ':', ipport);
    int port = stoi(ipport[1]);

    int peerfd = socket(AF_INET, SOCK_STREAM, 0);
    if (peerfd < 0) {
        printf("Error: In opening socket\n");
        return NULL;
    }

    struct sockaddr_in peer_address;
    peer_address.sin_family = AF_INET;
    peer_address.sin_addr.s_addr = INADDR_ANY;
    peer_address.sin_port = htons(port);

    if (connect(peerfd, (struct sockaddr*)&peer_address, sizeof(peer_address)) <
        0) {
        printf("Error: In creating connection\n");
        return NULL;
    }

    char buffer[BUFSIZ];
    bzero(buffer, BUFSIZ);

    string req = "download " + filepath + " " + to_string(chunknumber) + "\n";

    cout << req << endl;

    int n = write(peerfd, req.c_str(), req.size());
    if (n < 0) {
        printf("Couldn't send the request\n");
        return NULL;
    }

    int fd = open(
        "/mnt/LINUXDATA/bhanujggandhi/Learning/iiit/sem1/aos/assignment_3/"
        "learn/peer-to-peer/copied.txt",
        O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    long totalfilesize = getfilesize(filepath) - (chunknumber * CHUNKSIZE);
    int filesize = CHUNKSIZE;
    loff_t offset = chunknumber * CHUNKSIZE;
    ssize_t readbytes = 0;

    while (totalfilesize > 0 && filesize > 0 &&
           (readbytes = read(peerfd, buffer, BUFSIZ)) > 0) {
        ssize_t written = pwrite64(fd, buffer, readbytes, offset);
        offset += written;
        // memset(buffer, 0, BUFSIZ);
        filesize -= readbytes;
        totalfilesize -= readbytes;
    }

    close(fd);
    close(peerfd);

    printf("Chunk %d downloaded successfully from %d\n", chunknumber, port);

    return NULL;
}

void* downloadstart(void* arg) {
    DownloadPassThread transferdata = *(DownloadPassThread*)arg;

    vector<DownloadData*> pieceselection;
    for (auto x : transferdata.chunktomap) {
        DownloadData* dd = new DownloadData();
        dd->chunknumber = x.first;
        dd->srcfilepath = transferdata.srcfilepath;
        dd->filesha = transferdata.filesha;
        dd->peers = x.second;
        dd->destfilepath = transferdata.destinationpath;

        pieceselection.push_back(dd);
    }

    sort(pieceselection.begin(), pieceselection.end(), cmp);

    pthread_t th[DOWNLOAD_THREAD_POOL];
    pthread_mutex_init(&mutexDownQueue, NULL);
    pthread_cond_init(&condDownQueue, NULL);
    int breakcond = pieceselection.size();
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        check(pthread_create(&th[i], NULL, start_down_thread, &breakcond),
              "Failed to create the thread");
    }

    for (int i = 0; i < pieceselection.size(); i++) {
        pthread_mutex_lock(&mutexDownQueue);
        threadDownQueue.push(pieceselection[i]);
        pthread_cond_signal(&condDownQueue);
        pthread_mutex_unlock(&mutexDownQueue);
    }

    pthread_mutex_destroy(&mutexDownQueue);
    pthread_cond_destroy(&condDownQueue);

    printf("Download Completed Successfully!\n");
    return NULL;
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