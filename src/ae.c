#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include "ae.h"

aeEventLoop* aeCreateEventLoop(int setsize) {
    aeEventLoop* eventloop = (aeEventLoop*)malloc(sizeof(aeEventLoop));
    if(eventloop == NULL) {
        return NULL;
    }
    eventloop->maxfd = -1;
    eventloop->setsize = setsize;

    eventloop->events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*setsize);
    if(eventloop->events == NULL) {
        free(eventloop);
        return NULL;
    }
    if((eventloop->epollfd = epoll_create(1024)) < 0) {
        free(eventloop->events);
        free(eventloop); 
        return NULL;
    }

    eventloop->procs = (aeFileEvent*)malloc(sizeof(aeFileEvent)*setsize);
    if(eventloop->procs == NULL) {
        free(eventloop->events);
        free(eventloop);
        return NULL;
    }

    int i;
    for(i = 0; i < setsize; i++)
        eventloop->procs[i].mask = AE_NONE;

    return eventloop;
}

void aeDeleteEventLoop(aeEventLoop* eventloop) {
    close(eventloop->epollfd);
    free(eventloop->events);
    free(eventloop->procs);
    free(eventloop);
}

int aeCreateFileEvent(aeEventLoop* eventloop, int fd, int mask, 
        aeFileProc* proc, void* data) {
    if(fd >= eventloop->setsize) {
        errno = ERANGE;
        return AE_ERR;   
    } 

    struct epoll_event ev;
    ev.events = 0;
    ev.data.fd = fd;
    mask |= eventloop->procs[fd].mask; 
    if(mask & AE_READABLE) ev.events |= EPOLLIN;
    if(mask & AE_WRITABLE) ev.events |= EPOLLOUT;
    int op = (eventloop->procs[fd].mask == AE_NONE) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    if(epoll_ctl(eventloop->epollfd, op, fd, &ev) < 0) 
        return AE_ERR;

    if(fd > eventloop->maxfd)
        eventloop->maxfd = fd;

    aeFileEvent* fe = &eventloop->procs[fd];
    fe->mask |= mask;
    if(mask & AE_READABLE) fe->rfileProc = proc; 
    if(mask & AE_WRITABLE) fe->wfileProc = proc; 
    fe->clientData = data;

    return AE_OK;
}

void aeDeleteFileEvent(aeEventLoop* eventloop, int fd, int mask) {
    if(fd >= eventloop->setsize)    return;
    

    aeFileEvent* fe = &eventloop->procs[fd];
    if(fe->mask == AE_NONE) return;
    fe->mask &= ~mask;
    if(fd == eventloop->maxfd && fe->mask == AE_NONE) {
        int i;
        for(i = fd - 1; i >= 0; i--) {
            if(eventloop->procs[i].mask != AE_NONE) 
               break; 
        }

        eventloop->maxfd = i;
    }

    struct epoll_event ev;
    ev.events = 0;
    ev.data.fd = fd;
    if(fe->mask & AE_READABLE) ev.events |= EPOLLIN;
    if(fe->mask & AE_WRITABLE) ev.events |= EPOLLOUT;
    int op = (fe->mask == AE_NONE) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_ctl(eventloop->epollfd, op, fd, &ev); 
}

int aeMain(aeEventLoop *eventloop) {
    int i, nfds;

    if(eventloop->maxfd == -1) 
        return -1;

    nfds = epoll_wait(eventloop->epollfd, eventloop->events, eventloop->setsize, -1);
    for(i = 0; i < nfds; ++i) {
        struct epoll_event* ev = &eventloop->events[i];
        aeFileEvent* fe = &eventloop->procs[ev->data.fd];
        if((fe->mask & AE_READABLE) && (ev->events & EPOLLIN))
            fe->rfileProc(eventloop, ev->data.fd, fe->clientData);
        if((fe->mask & AE_WRITABLE) && (ev->events & EPOLLOUT))
            fe->wfileProc(eventloop, ev->data.fd, fe->clientData);
    }

    return nfds; 
}
