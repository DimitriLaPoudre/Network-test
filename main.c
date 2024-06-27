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
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

#define PORT 13216

#define NOTHING 0
#define NEW_MESSAGE 1
#define LEAVE 2

typedef struct message {
    char sender[256];
    char message[256];
    struct message *next;
} message_t;

typedef struct {
    message_t *list;
    char me[256];
    char user[256];
    char *balise;
    int canal;
    char *update;
} args_t;

static void putnbr_base(unsigned long nb, char const *base)
{
    int t_base = 0;

    t_base = strlen(base);
    if (nb >= t_base)
        putnbr_base(nb / t_base, base);
    addch(base[nb % t_base]);
}

static unsigned long getnbr_base(char *str, char const *base)
{
    unsigned long nb = 0;
    int len = strlen(base);

    for (int i = 0; str[i] && strchr(base, str[i]); i++)
        nb = nb * len + strcspn(base, (char[2]){str[i], '\0'});
    return nb;
}

static unsigned int get_ip_public(void)
{
    struct UPNPDev *devlist;
    struct UPNPDev *dev;
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lan_addr[64];
    int error = 0;
    char port_str[6];
    char externalIPAddress[40];
    unsigned int ip;
    char *ptr;

    snprintf(port_str, sizeof(port_str), "%u", PORT);
    devlist = upnpDiscover(1000, NULL, NULL, 1, 0, 2, &error);
    if (devlist == NULL) {
        endwin();
        printf("Erreur lors de la découverte des routeurs UPnP : %d\n", error);
        return 0;
    }
    dev = devlist;
    if (UPNP_GetValidIGD(dev, &urls, &data, lan_addr, sizeof(lan_addr)) != 1) {
        endwin();
        printf("Erreur lors de la sélection du routeur UPnP\n");
        freeUPNPDevlist(devlist);
        return 0;
    }

    if (UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress) != UPNPCOMMAND_SUCCESS) {
        endwin();
        printf("Erreur lors de l'obtention de l'adresse IP externe.\n");
        return 0;
    } else {
        if (externalIPAddress[0]) {
            ptr = externalIPAddress;
            ip = atoi(ptr) << 24;
            ptr = strchr(ptr, '.') + 1;
            ip += atoi(ptr) << 16;
            ptr = strchr(ptr, '.') + 1;
            ip += atoi(ptr) << 8;
            ptr = strchr(ptr, '.') + 1;
            ip += atoi(ptr);
        } else
            return 0;
    }

    error = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                port_str, port_str, lan_addr,
                                "Port Forwarding", "TCP", NULL, "10");
    if (error != UPNPCOMMAND_SUCCESS) {
        endwin();
        printf("Erreur lors de l'ajout du port forwarding : %d (%s)\n", error, strupnperror(error));
        return 0;
    }
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return ip;
}

unsigned int wait_connection(void)
{
    unsigned int code = get_ip_public();
    int server;
    int client;
    struct sockaddr_in data_addr = {0};
    unsigned int data_addr_len = sizeof(struct sockaddr_in);

    if (code == 0)
        return(0);
    printw("connection code:\n");
    putnbr_base(code, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    addch('\n');
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        endwin();
        perror("Erreur d'ouverture du socket");
        return(0);
    }
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(PORT);
    if (bind(server, (struct sockaddr *)&data_addr, sizeof(struct sockaddr_in)) < 0) {
        endwin();
        perror("Erreur de liaison");
        close(server);
        return(0);
    }
    if (listen(server, 1) < 0) {
        endwin();
        perror("Erreur d'écoute");
        close(server);
        return(0);
    }
    data_addr_len = sizeof(data_addr);
    client = accept(server, (struct sockaddr *)&data_addr, &data_addr_len);
    if (client < 0) {
        endwin();
        perror("Erreur d'acceptation");
        close(server);
        return(0);
    }
    close(server);
    return(client);
}

unsigned int connect_to_ip(char *code)
{
    unsigned int ip = getnbr_base(code, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    int server;
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in data_addr = {0};
    unsigned int data_addr_len = sizeof(struct sockaddr_in);
    char buffer[256] = {0};
    
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        endwin();
        perror("Erreur d'ouverture du socket");
        return(0);
    }
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(ip);
    data_addr.sin_port = htons(PORT);
    if (connect(server, (struct sockaddr *)&data_addr, sizeof(struct sockaddr_in)) < 0) {
        endwin();
        perror("Erreur de liaison");
        close(server);
        return(0);
    }
    return(server);
}

void *receive_message(void *args)
{
    args_t *info = args;
    message_t *message;
    int len = 0;

    while (1) {
        message = malloc(sizeof(message_t));
        if ((len = read(info->canal, message->message, 255)) < 0) {
            endwin();
            perror("Erreur de lecture");
            exit(EXIT_FAILURE);
        }
        if (len == 0) {
            endwin();
            printf("L'utilisateur a quitté la conversation.");
            exit(EXIT_SUCCESS);
        }
        message->message[len] = '\0';
        printw("Utilisateur: %s%c", message->message, message->message[len - 1] == '\n' ? '\0' : '\n');
    }
}

char is_empty(int fd, pthread_t thread)
{
    fd_set fds;
    struct timeval timeout;
    int ready = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    ready = select(fd + 1, &fds, NULL, NULL, &timeout);
    if (ready == -1) {
        endwin();
        perror("select");
        pthread_join(thread, NULL);
        exit(EXIT_FAILURE);
    } else if (ready > 0 && FD_ISSET(fd, &fds))
        return 0;
    else
        return 1;
}

char send_message(args_t *info, pthread_t thread)
{
    message_t *message;
    int len = 0;

    while (1) {
        message = malloc(sizeof(message_t));
        while (len < 255) {
            if (is_empty(info->canal, thread) == 1)
                continue;
            if (read(0, message->message, 1) < 0) {
                endwin();
                perror("Erreur d'écriture");
                pthread_join(thread, NULL);
                return(EXIT_FAILURE);
            }
            // print_history(info);
        }
        message->message[len] = '\0';
        if ((len = write(info->canal, message->message, 255)) < 0) {
            endwin();
            perror("Erreur d'écriture");
            pthread_join(thread, NULL);
            return(EXIT_FAILURE);
        }
        if (len == 0) {
            endwin();
            puts("L'utilisateur a quitté la conversation.");
            pthread_join(thread, NULL);
            return(EXIT_SUCCESS);
        }
        printw("Moi: %s%c", message->message, message->message[len - 1] == '\n' ? '\0' : '\n');
    }
}

int main(int ac, char **av)
{
    message_t message_list = {0};
    char balise = 0;
    char update = NOTHING;
    args_t args = {&message_list, {0}, {0}, &balise, 0, &update};
    pthread_t thread;

    initscr();
    noecho();
    curs_set(0);
    args.canal = (!av[1]) ? wait_connection() : connect_to_ip(av[1]);
    refresh();
    if (!args.canal)
        return(EXIT_FAILURE);
    // pthread_create(&thread, NULL, receive_message, &args);
    return send_message(&args, thread);
}