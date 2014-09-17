#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mysql.h>
#include <time.h>
#include "wsn.h"
#include "ae.h"
#include "anet.h"
#include "fann.h"

#define DEVICE_ID_LENGTH    10
#define MAX_QUEUE_LENGTH    100
#define FANN_CONFIG_FILE    "iaq_float.net"

#define MYSQL_SERVER        "localhost"
#define MYSQL_USER          "root"
#define MYSQL_PASSWD        "joe159357"
#define MYSQL_DATABASE      "iaq"

#define INSERT_DATA "INSERT INTO iaq_sensordata VALUES" \
                    "(DEFAULT, '%s', %.1f, %.1f, %d, %d, %d, %.2f, '%s')"

#define UPDATE_NODE "UPDATE iaq_sensornode SET temp=%.1f, humi=%.1f, " \
                    "iaqengine=%d, tgs2600=%d, tgs2602=%d, " \
                    "formaldehyde=%.2f, time='%s' WHERE node_id='%s'"

typedef struct clientData {
    char id[DEVICE_ID_LENGTH];
    float temp;
    float humi;
    int qs01;
    int tgs2600;
    int tgs2602;
} clientData;

typedef struct clientDataQueue {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int cur_num, cur_ind, cur_next, max_size;
    clientData data[MAX_QUEUE_LENGTH]; 
} clientDataQueue;

wsnServer server;
clientDataQueue queue = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .cur_num = 0,
    .cur_ind = 0,
    .cur_next = 0,
    .max_size = MAX_QUEUE_LENGTH
};
/*-------------------------------private function-----------------------------*/
static MYSQL *connectMySQL() {
    MYSQL *conn;
    char *server = MYSQL_SERVER;
    char *user = MYSQL_USER;
    char *password = MYSQL_PASSWD; 
    char *database = MYSQL_DATABASE;

    /* Connect to database */
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, 
                user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    return conn;
}
static void *processClientData(void *arg) {
    /* Connect to mysql */
    MYSQL *conn = connectMySQL(); 
     
    /* Create neuro network */
    struct fann *ann = fann_create_from_file(FANN_CONFIG_FILE);
    fann_type sensors[5];
    fann_type *calc_out;

    char now[32], query[256];

    while(1) {
        pthread_mutex_lock(&queue.mutex);
        while(!queue.cur_num) 
            pthread_cond_wait(&queue.cond, &queue.mutex);

        /* retrive data from queue */
        clientData data = queue.data[queue.cur_ind++]; 
        queue.cur_num--;
        queue.cur_ind %= queue.max_size; 
        pthread_mutex_unlock(&queue.mutex); 
    
        sensors[0] = data.temp / 100;
        sensors[1] = data.humi /100;
        sensors[2] = data.qs01 / 1000.0;
        sensors[3] = data.tgs2600 / 1000.0;
        sensors[4] = data.tgs2602 / 1000.0;
        calc_out = fann_run(ann, sensors);

        time_t tt = time(NULL);
        strftime(now, 32, "%F %T", gmtime(&tt));
        printf("[%s] %s %.1f %.1f %d %d %d --> %.2f\n", now, data.id, data.temp, 
                data.humi, data.qs01, data.tgs2600, data.tgs2602, calc_out[0]);

        /* INSERT */
        snprintf(query, 256, INSERT_DATA, data.id, data.temp, data.humi, 
                data.qs01, data.tgs2600, data.tgs2602, calc_out[0], now);
        if(mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(1);
        }

        /* UPDATE */
        snprintf(query, 256, UPDATE_NODE, data.temp, data.humi, data.qs01, 
                data.tgs2600, data.tgs2602, calc_out[0], now, data.id);
        if(mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(1);
        }
    }

    return NULL;
}

static void initWorkerThread() {
    pthread_t tid;

    if(pthread_create(&tid, NULL, processClientData, NULL) < 0) {
        perror("pthread_create error");
        exit(1);
    }
}

static void initServer() {
    server.port = WSN_PORT; 
    server.backlog = MAX_BACKLOG;
    server.setsize = MAX_SETSIZE;
    server.listenfd = anetCreateListenSock(server.port); 
    server.el = aeCreateEventLoop(server.setsize);  
    aeCreateFileEvent(server.el, server.listenfd, AE_READABLE, anetListenHandler, NULL);
}

/*-------------------------------public function------------------------------*/
wsnClient* createClient(int fd) {
    wsnClient* c = (wsnClient*)malloc(sizeof(wsnClient));
    c->fd = fd;
    c->nread = 0;
    c->nleft = MAX_BUFSIZE;

    return c;
}

void deleteClient(wsnClient* c) {
    aeDeleteFileEvent(server.el, c->fd, AE_READABLE | AE_WRITABLE);
    close(c->fd);

    free(c);
}

void processClient(wsnClient* c) {
    char *beg, *end;
    clientData data;
    
    beg = c->buf;
    while((end = strchr(beg, '$')) != NULL) {
        int ret = sscanf(beg, "%s %f %f %d %d %d$", data.id, &data.temp, 
                &data.humi, &data.qs01, &data.tgs2600, &data.tgs2602);
        
        if(ret == 6) {
            pthread_mutex_lock(&queue.mutex);
            queue.data[queue.cur_next++] = data;
            queue.cur_num++;
            queue.cur_next %= queue.max_size;
            pthread_cond_signal(&queue.cond);
            pthread_mutex_unlock(&queue.mutex);
        }
        beg = end + 1;
    } 
    c->nread -= (beg - c->buf);
    c->nleft += (beg - c->buf);
    
    if(beg != c->buf) {
        if(c->nread > 0)
            memmove(c->buf, beg, c->nread);
        memset(c->buf + c->nread, 0, c->nleft);
    }
}

int main(int argc, char **argv) {
    initServer();
    initWorkerThread();

    while(1) {
        if(aeMain(server.el) < 0)   break;
    }

    return 0;
}


