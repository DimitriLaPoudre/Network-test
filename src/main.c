#include "file.h"

static void putnbr_base(unsigned long nb, char const *base)
{
    int t_base = 0;

    t_base = strlen(base);
    if (nb >= t_base)
        putnbr_base(nb / t_base, base);
    addch(base[nb % t_base]);
}

static void getstr_base(unsigned long nb, char const *base, char *code)
{
    int t_base = 0;

    t_base = strlen(base);
    if (nb >= t_base)
        getstr_base(nb / t_base, base, code++);
    *code = base[nb % t_base];
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

    printw("Creating server...");
    refresh();
    snprintf(port_str, sizeof(port_str), "%u", PORT);
    devlist = upnpDiscover(1000, NULL, NULL, 1, 0, 2, &error);
    if (devlist == NULL) {
        endwin();
        fprintf(stderr, "Erreur lors de la découverte des routeurs UPnP : %d\n", error);
        return 0;
    }
    dev = devlist;
    if (UPNP_GetValidIGD(dev, &urls, &data, lan_addr, sizeof(lan_addr)) != 1) {
        endwin();
        fprintf(stderr, "Erreur lors de la sélection du routeur UPnP\n");
        freeUPNPDevlist(devlist);
        return 0;
    }

    if (UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress) != UPNPCOMMAND_SUCCESS) {
        endwin();
        fprintf(stderr, "Erreur lors de l'obtention de l'adresse IP externe.\n");
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
        fprintf(stderr, "Erreur lors de l'ajout du port forwarding : %d (%s)\n", error, strupnperror(error));
        return 0;
    }
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return ip;
}

unsigned int create_server(server_t *server_data, client_t *client_data)
{
    unsigned int code = get_ip_public();

    if (code == 0)
        return 0;
    getstr_base(code, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", server_data->code);
    server_data->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_data->socket < 0) {
        endwin();
        perror("Erreur d'ouverture du socket");
        return 0;
    }
    server_data->data_addr.sin_family = AF_INET;
    server_data->data_addr.sin_addr.s_addr = INADDR_ANY;
    server_data->data_addr.sin_port = htons(PORT);
    if (bind(server_data->socket, (struct sockaddr *)&server_data->data_addr, sizeof(struct sockaddr_in)) < 0) {
        endwin();
        perror("Erreur de liaison");
        close(server_data->socket);
        return 0;
    }
    if (listen(server_data->socket, 10) < 0) {
        endwin();
        perror("Erreur d'écoute");
        close(server_data->socket);
        return 0;
    }
    server_data->state = RUN;
    pthread_create(&server_data->server_thrd, NULL, &server, server_data);
    return connect_to_code(server_data->code, client_data);
}

unsigned int connect_to_code(char *code, client_t *client_data)
{
    unsigned int ip = getnbr_base(code, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    int server;
    struct sockaddr_in data_addr = {0};
    unsigned int data_addr_len = sizeof(struct sockaddr_in);
    
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        endwin();
        perror("Erreur d'ouverture du socket");
        return 0;
    }
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(ip);
    data_addr.sin_port = htons(PORT);
    if (connect(server, (struct sockaddr *)&data_addr, sizeof(struct sockaddr_in)) < 0) {
        endwin();
        perror("Erreur de liaison");
        close(server);
        return 0;
    }
    strncpy(client_data->code, code, 7);
    return server;
}

int main(int ac, char **av)
{
    client_t client_data = {NULL, {0}, 0, PTHREAD_MUTEX_INITIALIZER, OFF, 0, 0, 0, {0}, true};
    server_t server_data = {NULL, client_data.code, 0, sizeof(struct sockaddr_in), 0, OFF, 0};

    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, true);
    keypad(stdscr, true);
    client_data.server = (!av[1]) ? create_server(&server_data, &client_data) : connect_to_code(av[1], &client_data);
    if (!client_data.server) {
        endwin();
        return EXIT_FAILURE;
    }
    printw("caca");
    refresh();
    logical_loop(&client_data);
    endwin();
    return EXIT_SUCCESS;
}
