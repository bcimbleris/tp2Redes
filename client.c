#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct {
    int socket;
    int choice;
    struct sockaddr_storage storage;
} ClientInfo;

void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server IP> <server port>\n", argv[0]);
    printf("example: %s ipv4 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

void menu() {
    printf("0 - Sair\n");
    printf("1 - Senhor dos Aneis\n");
    printf("2 - Blade Runner 2049\n");
    printf("3 - Mad Max: Fury Road\n");
    printf("Escolha: ");
}

int main(int argc, char **argv) {
    if (argc < 4) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], argv[3], &storage)) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    while (1) {
        ClientInfo clientInfo;
        clientInfo.socket = s;
        clientInfo.storage = storage;

        menu();
        int clientResponse;
        scanf("%d", &clientResponse);

        if (clientResponse == 0) {
            break;
        }

        clientInfo.choice = clientResponse;

        size_t count = sendto(s, &clientInfo, sizeof(clientInfo), 0, addr, sizeof(storage));
        if (count != sizeof(clientInfo)) {
            logexit("sendto");
        }

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        socklen_t addrlen = sizeof(storage);
        for (int i = 0; i < 5; i++) {
            count = recvfrom(s, buf, BUFSZ - 1, 0, addr, &addrlen);
            if (count == -1) {
                logexit("recvfrom");
            }
            buf[count] = '\0'; // Null-terminate the received data
            printf("received: %s\n", buf);
        }
    }

    close(s);
    return 0;
}
