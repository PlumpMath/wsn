#ifndef _AE_H_
#define _AE_H_

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_OK 0
#define AE_ERR -1

struct aeEventLoop;

typedef void aeFileProc(struct aeEventLoop *eventloop, int fd, void *data);

typedef struct aeFileEvent {
    int mask;
    aeFileProc* rfileProc;
    aeFileProc* wfileProc;
    void *clientData;
} aeFileEvent;

typedef struct aeEventLoop {
    int maxfd;                      /* max file descriptor currently used */ 
    int setsize;                    /* max file descirptor tracked */ 
    int epollfd;
    struct epoll_event* events;
    aeFileEvent* procs;
} aeEventLoop;

aeEventLoop* aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop* eventloop);
int aeCreateFileEvent(aeEventLoop* eventloop, int fd, int mask, aeFileProc* proc, void* data);
void aeDeleteFileEvent(aeEventLoop* eventloop, int fd, int mask);
int aeMain(aeEventLoop *eventloop);

#endif
