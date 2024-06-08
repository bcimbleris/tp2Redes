#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server port>\n", argv[0]);
    printf("example: %s ipv4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

typedef struct {
    int socket;
    int choice;
    struct sockaddr_storage storage;
} ClientInfo;

const char *lotr_quotes[] = {
    "Even the smallest person can change the course of the future.",
    "There is only one Lord of the Ring, only one who can bend it to his will. "
    "And he does not share power.",
    "I wish the ring had never come to me.",
    "There’s some good in this world, Mr. Frodo, and it’s worth fighting for.",
    "Not all those who wander are lost."};

const char *br2049_quotes[] = {
    "I always told you, you're special.",
    "A child. Of woman born. Pushed into the world. Wanted. Loved.",
    "I hope you don't mind me taking the liberty.",
    "You've never seen a miracle.", "All the best memories are hers."};

const char *madmax_quotes[] = {"Oh what a day, what a lovely day!",
                               "Hope is a mistake.", "We are not things.",
                               "My name is Max. My world is fire and blood.",
                               "I live, I die. I live again."};

pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

void *monitor_clients(void *arg) {
    while (1) {
        sleep(4);
        pthread_mutex_lock(&client_count_mutex);
        printf("Clientes: %d\n", client_count);
        pthread_mutex_unlock(&client_count_mutex);
    }
    return NULL;
}

void send_quotes(int sock, const char *quotes[], struct sockaddr *caddr,
                 socklen_t caddrlen) {
    char send_buf[BUFSZ];
    for (int i = 0; i < 5; i++) {
        snprintf(send_buf, BUFSZ, "%s\n", quotes[i]);
        size_t send_count =
            sendto(sock, send_buf, strlen(send_buf), 0, caddr, caddrlen);
        if (send_count != strlen(send_buf)) {
            logexit("sendto");
        }
        sleep(3);
    }
}

void *client_thread(void *data) {
    ClientInfo *clientInfo = (ClientInfo *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&clientInfo->storage);
    socklen_t caddrlen = sizeof(clientInfo->storage);

    pthread_mutex_lock(&client_count_mutex);
    client_count++;
    pthread_mutex_unlock(&client_count_mutex);

    if (clientInfo->choice == 1) {
        send_quotes(clientInfo->socket, lotr_quotes, caddr, caddrlen);
    } else if (clientInfo->choice == 2) {
        send_quotes(clientInfo->socket, br2049_quotes, caddr, caddrlen);
    } else if (clientInfo->choice == 3) {
        send_quotes(clientInfo->socket, madmax_quotes, caddr, caddrlen);
    }

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    free(clientInfo);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    pthread_t monitor_tid;
    pthread_create(&monitor_tid, NULL, monitor_clients, NULL);
    pthread_detach(monitor_tid);

    while (1) {
        ClientInfo *clientInfo = malloc(sizeof(*clientInfo));
        if (!clientInfo) {
            logexit("malloc");
        }

        socklen_t caddrlen = sizeof(clientInfo->storage);
        clientInfo->socket = s;
        int recv_count =
            recvfrom(s, clientInfo, sizeof(*clientInfo), 0,
                     (struct sockaddr *)(&clientInfo->storage), &caddrlen);
        if (recv_count == -1) {
            logexit("recvfrom");
        }

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, clientInfo);
        pthread_detach(tid);
    }

    close(s);
    return 0;
}
