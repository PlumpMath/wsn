#ifndef _WSN_H_
#define _WSN_H_
#include "ae.h"

#define WSN_PORT    9750 
#define MAX_BUFSIZE 4096
#define MAX_SETSIZE 1000

typedef struct wsnServer {
    int port;
    int backlog;
    int listenfd;
    int setsize;
    aeEventLoop *el;
} wsnServer;

typedef struct wsnClient {
    int fd;
    int nread;
    int nleft;
    char buf[MAX_BUFSIZE];
} wsnClient;

wsnClient* createClient(int fd);
void deleteClient(wsnClient* c);
void processClient(wsnClient* c);

#endif
