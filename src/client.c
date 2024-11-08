#include "file.h"

static void *receive_message(void *args)
{
    client_t *client_data = args;
    static message_t *get_message;

    while (client_data->state == RUN) {
        while (fd_is(client_data->server, 0, 1)) {
            get_message = malloc(sizeof(message_t));
            if (read(client_data->server, get_message, sizeof(message_t)) != sizeof(message_t)) {
                /* server fuck up */
            }
            decrypt(get_message->message, client_data->code, get_message->iv);
            pthread_mutex_lock(&client_data->mutex);
            get_message->next = client_data->list;
            client_data->list = get_message;
            if (client_data->act_msg == client_data->index_msg)
                client_data->act_msg++;
            client_data->index_msg++;
            client_data->refresh = true;
            pthread_mutex_unlock(&client_data->mutex);
        }
    }
    close(client_data->server);
}

void display_message(char msg[256], client_t *client_data)
{
    static int height;
    static int width;

    clear();
    getmaxyx(stdscr, height, width);
    move(0, ((width - 13) / 2));
    printw("CODE: ");
    addnstr(client_data->code, 7);
    addch('\n');

    for (message_t *tmp = client_data->list; tmp && stdscr->_cury < height; tmp = tmp->next)
        printw("[%s]: %s\n", tmp->sender, tmp->message);

    move(height - 1 - (256 + width - 1 )/ width, 0);
    {
        char line[width + 1];

        memset(line, '-', width);
        line[width] = '\0';
        addstr(line);
    }
    printw("%s\n", msg);
    refresh();
}

void send_message(char msg[256], client_t *client_data)
{
    static message_t message = {0};

    strncpy(message.message, msg, 256);
    for (int i = 0; i < 256; i++)
        message.iv[i] = rand() & 0xFF;
    encrypt(message.message, client_data->code, message.iv);
    message.command = 0x00;
    write(client_data->server, &message, sizeof(message_t));
}

void logical_loop(client_t *client_data)
{
    int c;
    char message[256] = {0};
    int len = 0;
    int index = 0;
    
    client_data->state = RUN;
    pthread_create(&client_data->client_thrd, NULL, &receive_message, &client_data);
    while (client_data->state == RUN) {
        while ((c = getch()) != ERR) {
            if (c == KEY_BACKSPACE || c == 127) {
                if (index > 0) {
                    index--;
                    len--;
                    message[index] = '\0';
                    client_data->refresh = true;
                }
                continue;
            }
            if (c == KEY_UP && client_data->act_msg != 0) {
                client_data->act_msg--;
                client_data->refresh = true;
                continue;
            }
            if (c == KEY_DOWN && client_data->act_msg != client_data->index_msg) {
                client_data->act_msg++;
                client_data->refresh = true;
                continue;
            }
            if (c == '\n') {
                send_message(message, client_data);
                memset(message, 0, 256);
                index = 0;
                len = 0;
                client_data->refresh = true;
                continue;
            }
            if (len != 256 && c >= 32 && c <= 126) {
                message[index] = c;
                index++;
                len++;
                client_data->refresh = true;
                continue;
            }
        }
        if (client_data->refresh == true) {
            display_message(message, client_data);
            client_data->refresh = false;
        }
    }
}
