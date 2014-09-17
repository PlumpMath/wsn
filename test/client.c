#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static int str_cli(FILE* fp, int fd);

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        exit(1); 
    }


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
        perror("inet_pton error");
        exit(1);
    }
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }


    str_cli(stdin, sockfd);

    return 0;
}

int str_cli(FILE* fp, int fd) {
    int n;
    char buf[1024];

    while((n = read(fileno(fp), buf, 1024)) != 0) {
        write(fd, buf, n - 1);
    }

    close(fd);
    return 0;
}
