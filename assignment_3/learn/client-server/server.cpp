#include <arpa/inet.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <queue>
#include <string>

#define THREAD_POOL_SIZE 5

using namespace std;

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
queue<int*> thread_queue;

void error(const char* msg);
void check(int status, string msg);
void* handle_connection(void* p_client_socket);
void* thread_function(void* arg);

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Error, No PORT Provided\n");
        exit(1);
    }

    // Adress Family, Protocol to send(UDP/TCP), Protocol (Usually 0 as it is
    // unspecified)
    int server_socket;
    check(server_socket = socket(AF_INET, SOCK_STREAM, 0),
          "ERROR in Opening Socket");

    // Making sure all the content inside the newly created Internet Socket
    // Address struct is clear and set to 0
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));

    int portno = atoi(argv[1]);

    // Mentioning the address family in the struct
    server_addr.sin_family = AF_INET;

    // This statement sets any random IP address for socket connection
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Sending port number to the struct but the thing is computer way of
     * handling port numbers and network way of handling port number is
     * incompatible */
    server_addr.sin_port = htons(portno);

    /* Now to send this struct over network in bind call we have to convert it
     * into network stream, so we are using sockaddr struct for that.

     * Now this is passed into the bind system call with the file descriptor
     of socket
      */

    check(bind(server_socket, (struct sockaddr*)&server_addr,
               sizeof(server_addr)),
          "ERROR on binding");

    /* Listen call take socket file descriptor as well as maximum number of
     * requests to store in the backlog queue */
    check(listen(server_socket, 100), "Listen Failed");

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    while (true) {
        printf("Waiting for connections...\n");

        struct sockaddr_in client_addr;
        socklen_t clilen = sizeof(client_addr);
        /* Now when we are listening and we get a request, to accept the request
         * we get a new file descriptor which will help us in communication, now
         * all the messages would be passed to the client using this new file
         * descriptor
         */
        int client_socket;
        check(client_socket = accept(server_socket,
                                     (struct sockaddr*)&client_addr, &clilen),
              "ERROR on Accept");

        printf("Server: Got connection from %s port %d on the socket number\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* Multithreading the handling request, whenever server will accept a
         * new request a separate thread will be created for every request to
         * serve. It is quite useful when there a lot of synchronous requests.
         * If you open two terminals, first T1 and then T2. Send a request using
         * T2 it will wait until T1 is completed as server has no threads. But
         * when multithreading is implemented it will respond to the T2 request
         * right away. */

        // pthread_t thd;

        int* pclient = (int*)malloc(sizeof(int));
        *pclient = client_socket;

        pthread_mutex_lock(&mutex);
        thread_queue.push(pclient);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);

        /* Thread creating takes thread pointer, the function that thread will
         * perform and then all the arguments of that function. The function
         * that will handle must return the void pointer and takes in all the
         * arguments as void pointer, that is why we have changed the arguments
         * and everything. */

        // pthread_create(&thd, NULL, handle_connection, pclient);
        // handle_connection(pclient);
    }

    close(server_socket);

    return 0;
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

void* thread_function(void* arg) {
    while (true) {
        int* pclient;
        pthread_mutex_lock(&mutex);
        if (thread_queue.empty()) {
            pclient = NULL;
            pthread_cond_wait(&condition_var, &mutex);
            if (!thread_queue.empty()) {
                pclient = thread_queue.front();
                thread_queue.pop();
            }
        } else {
            pclient = thread_queue.front();
            thread_queue.pop();
        }
        pthread_mutex_unlock(&mutex);

        if (pclient != NULL) {
            handle_connection(pclient);
        }
    }
}

void* handle_connection(void* p_client_socket) {
    int client_socket = *((int*)p_client_socket);
    free(p_client_socket);
    char buffer[BUFSIZ];
    size_t bytes_read = 0;
    int msgsize = 0;
    char actualpath[PATH_MAX + 1];

    while ((bytes_read = read(client_socket, buffer + msgsize,
                              sizeof(buffer) - msgsize - 1)) > 0) {
        msgsize += bytes_read;
        if (msgsize > BUFSIZ - 1 or buffer[msgsize - 1] == '\n') break;
    }
    check(bytes_read, "recv error");
    buffer[msgsize - 1] = 0;

    printf("REQUEST: %s\n", buffer);
    fflush(stdout);
    if (realpath(buffer, actualpath) == NULL) {
        printf("ERROR: bad path %s\n", buffer);
        close(client_socket);
        return NULL;
    }

    FILE* fp = fopen(actualpath, "r");
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
    printf("Closing connection\n");

    return NULL;
}