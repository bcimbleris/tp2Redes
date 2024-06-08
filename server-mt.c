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

//os dados do cliente que serao passados para a thread no void pointer
struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

//dispara uma thread para novos clientes, a funcao e void pointer pela definicao de como a thread e utilizada
void *client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    while (1) {
        socklen_t caddrlen = sizeof(cdata->storage);
        // Alteração nas funções de recebimento e envio para UDP
        ssize_t count = recvfrom(cdata->csock, buf, BUFSZ - 1, 0, caddr, &caddrlen);
        if (count == -1) {
            perror("recvfrom");
            break; // Se ocorrer um erro, saia do loop
        }

        printf("[msg] %s, %zd bytes: %s\n", caddrstr, count, buf);

        count = sendto(cdata->csock, buf, strlen(buf), 0, caddr, caddrlen);
        if (count == -1) {
            perror("sendto");
            break; // Se ocorrer um erro, saia do loop
        }
    }

    close(cdata->csock);
    free(cdata); // Liberando a memória alocada para a struct client_data
    //fecha a thread
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

    int s;
    s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            perror("accept");
            continue; // Se ocorrer um erro, continue esperando por novas conexões
        }

        //inicializando a struct e alocando espaço na memoria para a struct
        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            perror("malloc");
            continue; // Se ocorrer um erro, continue esperando por novas conexões
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        //instancia o tipo da thread. os parametros sao: tid é o identificador da thread, opcoes que nao serao utilizadas, a funcao utilizada e os dados a serem passados para a thread 
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
