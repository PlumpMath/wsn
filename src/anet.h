#ifndef _ANET_H_
#define _ANET_H_
#include "ae.h"

#define MAX_BACKLOG 100

int anetCreateListenSock(int port);
void anetListenHandler(aeEventLoop *eventloop, int fd, void *data);
void anetClientHandler(aeEventLoop *eventloop, int fd, void *data);

#endif
