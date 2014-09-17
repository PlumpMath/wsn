#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


#define MAX_BACKLOG 10
#define MAX_EVENTS 10
static void str_echo(int fd);

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

int main() {
    int nfds, connfd, listenfd, epollfd;
    struct sockaddr_in cliaddr;
    socklen_t addrlen;

    struct epoll_event ev, events[MAX_EVENTS];

    listenfd = anetCreateListenSock(9751);
    if((epollfd = epoll_create(10)) < 0) {
        perror("epoll_create error");
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
        perror("epoll_ctl error");
        exit(1);
    }
    
    while(1) {
        if((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) < 0) {
            perror("epoll_wait error");
            exit(1);
        }
        int n;
        for(n = 0; n < nfds; n++) {
            if(events[n].data.fd == listenfd) {
                if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &addrlen)) < 0) {
                    perror("accept error");
                    exit(1);
                }

                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                if(epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                    perror("epoll_ctl error");
                    exit(1);
                }
            } else {
                str_echo(events[n].data.fd);
            }
        }
    }

    return 0;
}

static void str_echo(int fd) {
    int n;
    char buf[128];
    printf("Client connect\n");
    while(1) {
        n = read(fd, buf, 128);
        if(!n)  break;
        
        buf[n] = 0;
        printf("Received: %s\n", buf);
    }

    printf("Client disconnect\n");
    close(fd);
}
