#include <linux/limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage %s hostname port\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR in Opening PORT");

    struct hostent *server = gethostbyname(argv[1]);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error Connecting");

    char buffer[256];
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    printf("%s\n", buffer);

    int n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) error("ERROR writing to socket");

    char buff[BUFSIZ];
    bzero(buff, BUFSIZ);
    n = read(sockfd, buff, BUFSIZ);

    if (n < 0) error("ERROR reading from socket");

    printf("%s\n", buff);
    close(sockfd);
    return 0;
}