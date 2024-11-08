#include "file.h"

char fd_is(int fd, char can_read, char have_write)
{
    static fd_set readfds;
    static fd_set writefds;
    static struct timeval timeout = {0, 0};
    static int ret;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    ret = 0;
    FD_SET(fd, &readfds);
    FD_SET(fd, &writefds);
    if (select(fd + 1, (have_write) ? &readfds : NULL,
    (can_read) ? &writefds : NULL, NULL, &timeout) < 0)
        puts("Error select()");
    if (can_read && FD_ISSET(fd, &writefds))
        ret += 1;
    if (have_write && FD_ISSET(fd, &readfds))
        ret += 2;
    return ret;
}

static void accept_client(server_t *server_data)
{
    static int client_socket;
    static int index = 1;
    static namelist_t *new_name;
    static message_t new_connexion = {0};
    static struct timespec ts;

    while (fd_is(server_data->socket, 0, 1)) {
        client_socket = accept(server_data->socket,
        (struct sockaddr *)&(server_data->data_addr), &(server_data->data_addr_len));
        if (client_socket < 0)
            return;
        new_name = malloc(sizeof(namelist_t));
        if (new_name == 0)
            return;
        new_name->active = 1;
        clock_gettime(CLOCK_REALTIME, &ts);
        sprintf(new_name->name, "#%lld", (long) ts.tv_sec * 1000000000LL + ts.tv_nsec);
        new_name->socket = client_socket;
        new_name->next = server_data->namelist;
        server_data->namelist = new_name;
        index++;
        sprintf(new_connexion.message, "%s join the chat.", new_name->name);
        encrypt(new_connexion.message, server_data->code);
        for (namelist_t *tmp = new_name; tmp; tmp = tmp->next)
            write(tmp->socket, &new_connexion, sizeof(message_t));
    }
}

static void relay(server_t *server_data)
{
    static message_t message_to_relay;

    for (namelist_t *tmp = server_data->namelist; tmp; tmp = tmp->next) {
        while (fd_is(tmp->socket, 0, 1)) {
            if (read(tmp->socket, &message_to_relay, sizeof(message_t)) != sizeof(message_t)) {
                tmp->active = 0;
                break;
            }
            strncpy(message_to_relay.sender, tmp->name, 256);
            message_to_relay.command |= 0x01;
            write(tmp->socket, &message_to_relay, sizeof(message_t));
            message_to_relay.command ^= 0x01;
            for (namelist_t *tmp2 = server_data->namelist; tmp2; tmp2 = tmp2->next) {
                if (tmp->active && tmp != tmp2)
                    write(tmp2->socket, &message_to_relay, sizeof(message_t));
            }
        }
    }
}

static void disconnect(server_t *server_data)
{
    namelist_t *prev = NULL;

    for (namelist_t *tmp = server_data->namelist; tmp;) {
        if (!tmp->active) {
            if (prev) {
                prev->next = tmp->next;
                close(tmp->socket);
                free(tmp);
                tmp = prev;
                tmp = tmp->next;
            } else {
                server_data->namelist = tmp->next;
                close(tmp->socket);
                free(tmp);
                tmp = server_data->namelist;
                continue;
            }
        } else {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

void *server(void *args)
{
    server_t *server_data = args;

    while (server_data->state == RUN) {
        accept_client(server_data);
        relay(server_data);
        disconnect(server_data);
    }
    /* free toute les liste du server */
    close(server_data->socket);
    server_data->state = OFF;
    return NULL;
}
