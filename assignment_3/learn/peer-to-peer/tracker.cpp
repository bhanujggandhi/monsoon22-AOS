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

#define THREAD_POOL_SIZE 8

using namespace std;

struct User {
    string userid;
    string password;
    string address;
    unordered_set<string> mygroups;
    unordered_set<string> groups;
};
struct Group {
    string groupid;
    string adminid;
    unordered_set<string> members;
    unordered_set<string> files;
    unordered_set<string> requests;
};
struct FileStr {
    string filename;
    string SHA;
    long filesize;
    unordered_set<string> userids;
    unordered_set<string> users;
};

// Globals
pair<string, int> connectioninfo;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
queue<int*> thread_queue;
unordered_map<string, User*> usertomap;
unordered_map<string, Group*> grouptomap;
unordered_map<string, bool> loggedin_map;
unordered_map<string, FileStr*> filetomap;

// Functions
void err(const char* msg);
void check(int status, string msg);
void splitutility(string str, char del, vector<string>& pth);
void getipport(char* filepath, int number);
void* server_function(void* arg);
void client_function(const char* request, int CLIENTPORT);
void* start_thread(void* arg);
void download_file(char* path, int client_socket);
void* handle_connection(void* socket);

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "USAGE: ./tracker <path-to-trackerinfo.txt> <tracker-number>"
             << endl;
        exit(1);
    }

    getipport(argv[1], stoi(argv[2]));
    cout << "\x1B[2J\x1B[H";

    printf("Connected to: %s on port %d\n", connectioninfo.first.c_str(),
           connectioninfo.second);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, NULL);

    while (1) {
        char request[255];
        memset(request, 0, 255);
        fflush(stdin);
        fgets(request, 255, stdin);
        vector<string> parsedReq;
        string req(request);
        if (req == "quit\n") {
            exit(0);
        }
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

void getipport(char* filename, int number) {
    char resolvedpath[_POSIX_PATH_MAX];
    if (realpath(filename, resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        exit(1);
    }

    std::ifstream file(resolvedpath);
    int num = number;
    string ipport;
    while (num--) {
        if (file.is_open()) {
            string line;
            getline(file, line);
            ipport = line;
        } else {
            printf("Cannot open the file!\n");
            exit(1);
        }
    }
    file.close();

    vector<string> ipportsplit;
    splitutility(ipport, ':', ipportsplit);
    connectioninfo.first = ipportsplit[0];
    connectioninfo.second = stoi(ipportsplit[1]);
}

void* server_function(void* arg) {
    // // Parse IP:PORT
    // char* ipport = (char*)arg;
    // vector<string> ipportsplit;
    // string ipportstr(ipport);
    // splitutility(ipportstr, ':', ipportsplit);

    int portno = connectioninfo.second;

    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR: Socket Cannot be Opened!");

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, connectioninfo.first.c_str(),
                  &server_addr.sin_addr) == 0) {
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

void* handle_connection(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    char request[2000000];
    bzero(request, 2000000);

    size_t bytes_read;
    int req_size = 0;

    while ((bytes_read = read(client_socket, request + req_size,
                              sizeof(request) - req_size - 1)) > 0) {
        req_size += bytes_read;
        if (req_size > 2000000 - 1 or request[req_size - 1] == '\n') break;
    }
    check(bytes_read, "recv error");
    request[req_size - 1] = 0;

    vector<string> reqarr;
    string req(request);
    splitutility(req, ' ', reqarr);
    if (reqarr.empty()) {
        string msg = "Something went wrong\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    }

    if (reqarr[0] == "create_user") {
        string userid = reqarr[1];
        string password = reqarr[2];
        string address = reqarr[3];

        if (usertomap.find(userid) != usertomap.end()) {
            string msg =
                "1:User ID already exists\nEither Login or create an account "
                "with unique User ID\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        User* newUser = new User();
        newUser->userid = userid;
        newUser->password = password;
        newUser->address = address;
        usertomap.insert({userid, newUser});
        loggedin_map[userid] = true;
        string res = "2:" + userid + ":User created successfully!\n";
        write(client_socket, res.c_str(), res.size());
    } else if (reqarr[0] == "login") {
        string userid = reqarr[1];
        string password = reqarr[2];
        string address = reqarr[3];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg =
                "1:User ID does not exist\nPlease create an account to "
                "continue\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (password == usertomap[userid]->password) {
            loggedin_map[userid] = true;
            auto currUser = usertomap[userid];
            currUser->address = address;
            string res = "2:" + userid + ":User logged in successfully!\n";
            write(client_socket, res.c_str(), res.size());
            return NULL;
        }

        loggedin_map[userid] = false;
        string msg = "1:Password is incorrect, please try again\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "create_group") {
        string groupid = reqarr[1];
        string userid = reqarr[2];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        if (grouptomap.find(groupid) != grouptomap.end()) {
            string msg =
                "1:Group id already exists, please enter a unique one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        Group* newGroup = new Group();
        newGroup->adminid = userid;
        newGroup->groupid = groupid;
        newGroup->members.insert(userid);

        grouptomap.insert({groupid, newGroup});

        auto* curruser = usertomap[userid];
        curruser->groups.insert(groupid);
        curruser->mygroups.insert(groupid);

        string msg = "2:" + groupid + ":Group created successfully\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else if (reqarr[0] == "join_group") {
        string groupid = reqarr[1];
        string userid = reqarr[2];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->members.find(userid) != currGroup->members.end()) {
            string msg = "1:User is already a member of the group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        currGroup->requests.insert(userid);
        printf("Group size: %ld", currGroup->requests.size());
        string msg = "2:" + groupid + ":Request sent successfully\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else if (reqarr[0] == "leave_group") {
        string groupid = reqarr[1];
        string userid = reqarr[2];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->members.find(userid) == currGroup->members.end()) {
            string msg = "1:User is not the member of this group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (currGroup->adminid == userid) {
            string msg = "1:Admin cannot leave the group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        currGroup->members.erase(userid);
        string msg = "2:" + groupid + ":Group left successfully\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else if (reqarr[0] == "list_requests") {
        string groupid = reqarr[1];
        string userid = reqarr[2];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->adminid != userid) {
            string msg = "1:User does not have admin rights\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        string greqParse = "";
        long i = 1;
        for (auto uid : currGroup->requests) {
            greqParse += to_string(i) + ". " + uid + "\n";
            i++;
        }
        string msg = "2:Pending Requests:\n" + greqParse;
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "accept_request") {
        string groupid = reqarr[1];
        string userid = reqarr[2];
        string curruserid = reqarr[3];

        if (usertomap.find(userid) == usertomap.end() or
            usertomap.find(curruserid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[curruserid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->requests.find(userid) == currGroup->requests.end()) {
            string msg = "1:User has not requested to join the group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (currGroup->adminid != curruserid) {
            string msg = "1:You do not have admin rights for this group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        currGroup->requests.erase(userid);
        currGroup->members.insert(userid);

        string msg =
            "2:" + groupid + ":" + userid + " is the member of the group now\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else if (reqarr[0] == "list_groups") {
        string userid = reqarr[1];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        string greqParse = "";
        long i = 1;
        for (auto gid : grouptomap) {
            greqParse += to_string(i) + ". " + gid.first + "\n";
            i++;
        }
        string msg = "2:All Groups:\n" + greqParse;
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "list_files") {
        string groupid = reqarr[1];
        string userid = reqarr[2];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->members.find(userid) == currGroup->members.end()) {
            string msg = "1:User is not a member of this group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        string greqParse = "";
        long i = 1;
        for (auto fid : currGroup->files) {
            greqParse += to_string(i) + ". " + fid + "\n";
            i++;
        }
        string msg = "2:All Files:\n" + greqParse;
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "upload_file") {
        string filename = reqarr[1];
        string groupid = reqarr[2];
        string sha = reqarr[3];
        long filesize = stol(reqarr[4]);
        string userid = reqarr[5];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->members.find(userid) == currGroup->members.end()) {
            string msg =
                "1:You are not the member of the group! Please send the "
                "request to join!\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (filetomap.find(filename) != filetomap.end()) {
            auto currFile = filetomap[filename];
            // if (currFile->SHA != sha) {
            //     string msg =
            //         "1:Group has already a different file with same file
            //         name! " "Please choose a unique file name!\n";
            //     write(client_socket, msg.c_str(), msg.size());
            //     return NULL;
            // }

            currFile->userids.insert(userid);
            currFile->users.insert(usertomap[userid]->address);
            string msg = "2:" + groupid + ":File uploaded succesfully";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        FileStr* newfile = new FileStr();

        newfile->filename = filename;
        newfile->filesize = filesize;
        newfile->SHA = sha;
        newfile->users.insert(usertomap[userid]->address);
        newfile->userids.insert(userid);

        filetomap.insert({filename, newfile});

        currGroup->files.insert(filename);

        string msg = "2:" + groupid + ":File uploaded succesfully";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;

    } else if (reqarr[0] == "download_file") {
        string groupid = reqarr[1];
        string filepath = reqarr[2];
        string userid = reqarr[4];

        if (usertomap.find(userid) == usertomap.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (loggedin_map[userid] == false) {
            string msg = "1:Please login first\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (grouptomap.find(groupid) == grouptomap.end()) {
            string msg =
                "1:Group id does not exist. Please enter a valid one\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currGroup = grouptomap[groupid];

        if (currGroup->members.find(userid) == currGroup->members.end()) {
            string msg = "1:User is not a member of this group\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        if (filetomap.find(filepath) == filetomap.end() or
            currGroup->files.find(filepath) == currGroup->files.end()) {
            string msg = "1:File does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        auto currFile = filetomap[filepath];
        string res = "S";
        long countusers = 0;
        for (auto uid : currFile->userids) {
            auto peer = usertomap[uid];
            if (loggedin_map[uid] == false) {
                continue;
            }

            if (currFile->users.find(peer->address) == currFile->users.end()) {
                continue;
            }

            res += " " + peer->address;
            countusers++;
        }
        if (countusers == 0) {
            string msg = "1:File is currently unavailable\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }

        write(client_socket, res.c_str(), res.size());

        return NULL;

    } else if (reqarr[0] == "logout") {
        if (loggedin_map.find(reqarr[1]) == loggedin_map.end()) {
            string msg = "1:User does not exist\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        if (loggedin_map[reqarr[1]] == false) {
            string msg = "1:User is not logged in\n";
            write(client_socket, msg.c_str(), msg.size());
            return NULL;
        }
        loggedin_map[reqarr[1]] = false;
        string msg = "2:" + reqarr[1] + ":Logged out successfully!\n";
        write(client_socket, msg.c_str(), msg.size());
        return NULL;
    } else {
        string res = "1:Please enter a valid command";
        write(client_socket, res.c_str(), res.size());
        return NULL;
    }

    return NULL;
}