#ifndef FILE_H_
#define FILE_H_
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <net/if.h>
    #include <ifaddrs.h>
    #include <sys/select.h>
    #include <ncurses.h>
    #include <pthread.h>
    #include <time.h>
    #include <miniupnpc/miniupnpc.h>
    #include <miniupnpc/upnpcommands.h>
    #include <miniupnpc/upnperrors.h>

    #define PORT 13216

enum state_e {
    OFF,
    STOP,
    RUN
};

typedef struct namelist_s {
    char name[256];
    char active;
    int socket;
    struct namelist_s *next;
} namelist_t;

typedef struct message {
    char sender[256];
    char message[256];
    char command;
    struct message *next;
} message_t;

typedef struct {
    message_t *list;
    char me[256];
    int server;
    pthread_mutex_t mutex;
    char state;
    pthread_t client_thrd;
    int index_msg;
    int act_msg;
    char code[7];
    char refresh;
} client_t;

typedef struct {
    namelist_t *namelist;
    char *code;
    int socket;
    struct sockaddr_in data_addr;
    unsigned int data_addr_len;
    char state;
    pthread_t server_thrd;
} server_t;

unsigned int connect_to_code(char *code, client_t *client_data);

void logical_loop(client_t *client_data);

void *server(void *args);
char fd_is(int fd, char can_read, char have_write);

char *encrypt(char *str, char *key);
char *decrypt(char *str, char *key);

#endif
