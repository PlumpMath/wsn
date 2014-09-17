#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "wsn.h"
#include "anet.h"

static int anetNonBlock(int fd);
static int anetEnableNoDelay(int fd);
static int anetKeepAlive(int fd, int interval);

int anetCreateListenSock(int port) {
    int listenfd;
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {    
        perror("bind error");
        exit(1);
    }
    
    if(listen(listenfd, MAX_BACKLOG) < 0) {
        perror("listen error");
        exit(1);
    }
    
    return listenfd;
}

void anetListenHandler(aeEventLoop *eventloop, int fd, void *data) {
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if((connfd = accept(fd, (struct sockaddr*)&cliaddr, &addrlen)) < 0) {
        perror("accept error");
        exit(1);
    }
    wsnClient* c = createClient(connfd);
    if(c == NULL) {
        close(connfd);
        return;
    }
    aeCreateFileEvent(eventloop, connfd, AE_READABLE, anetClientHandler, c);
}

void anetClientHandler(aeEventLoop* eventloop, int fd, void* data) {
    wsnClient* c = (wsnClient*)data;
    int n = read(fd, c->buf + c->nread, c->nleft);
    if(n < 0) {
        if(errno == EAGAIN)
            n = 0;
        else {
            perror("read error");
            deleteClient(c);
            return;
        }
    } else if(n == 0) {
        printf("Client close connection: %d\n", fd);
        deleteClient(c);
        return;
    } else {
        c->nread += n;
        c->nleft -= n;
        processClient(c);
    }
}

static int anetNonBlock(int fd) {
    int flags;

    if((flags = fcntl(fd, F_GETFL)) < 0) {
        perror("fcntl(F_GETFL) error");
        return -1;
    }
    
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) error");
        return -1;
    }

    return 0;
}
