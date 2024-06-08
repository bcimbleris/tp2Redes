#include "common.h"

#include <arpa/inet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    double latitude;
    double longitude;
} Coordinate;
// Exemplo de como rodar o programa, aparece caso o programa seja usado errado
void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server IP> <server port>\n", argv[0]);
    printf("example: %s ipv4 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main(int argc, char **argv) {
label: ;
    // criando coordenada do cliente -19.92380718406128, -43.93492600471344
    Coordinate coordCli = {-19.9238, -43.9349};
    // verificando se o programa foi utilizado de maneira correta
    if (argc < 4) {
        usage(argc, argv);
    }
    struct sockaddr_storage storage;
    // retorna 0 quando funciona,recebe endereço do servidor, porto e
    // ponteiro para o sockaddr_storage que vai ser inicializado
    if (0 != addrparse(argv[1], argv[2], argv[3], &storage)) {
        usage(argc, argv);
    }
    // abre um socket de conexão TCP
    int s;
    s = socket(storage.ss_family, SOCK_DGRAM, 0);
    // verificando se deu erro ao abrir o socket, toda vez que o socket dá
    // erro retorna -1
    if (s == -1) {
        logexit("socket");
    }
    // uma interface que instancia um ponteiro e é feito o cast para o tipo
    // e coloca dentro da variável
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // menu inicial
    int clientResponse;

    printf("0 - Sair\n");
    printf("1 - Buscar corrida\n");
    scanf("%d", &clientResponse);
    switch (clientResponse) {
    case 0: {
        break;
    }
    case 1: {
        // chamando a função connect, toda vez que ela deu certo retorna 0,
        // portanto já é feito o tratamento de erro
        // passa o socket s, o endereço do servidor e o tamanho dessa estrutura
        if (0 != connect(s, addr, sizeof(storage))) {
            logexit("connect");
        }
        // endereço do endereço é guardado nessa variável
        char addrstr[BUFSZ];
        // imprime o endereço nessa string, recebe endereço, string e tamanho do
        // buffer
        addrtostr(addr, addrstr, BUFSZ);

        // comunicação do cliente com servidor
        char buf[BUFSZ];
        // inicializa o buffer com 0
        memset(buf, 0, BUFSZ);

        // socket, o dado que vai mandar, o número de bytes que vai mandar e flag
        size_t count = sendto(s, &coordCli, sizeof(Coordinate), 0, addr, sizeof(struct sockaddr));
        if (count != sizeof(Coordinate)) {
            logexit("send");
        }

        memset(buf, 0, BUFSZ);

        // fica recebendo dados do servidor até terminar a conexão
        while (1) {

            // recebe a resposta do servidor no socket s, coloca o dado no buff,
            // o tanto de dado que vai receber no bufsz e flag
            count = recvfrom(s, buf, BUFSZ, 0, addr ,sizeof(struct sockaddr));
            if (strcmp(buf, "Não foi encontrado um motorista") == 0) {
                puts(buf);
                close(s);
                memset(buf, 0, BUFSZ);
                goto label;
            }

            // caso em que nada foi recebido
            if (count == 0) {

                // Connection terminated.
                break;
            }
            puts(buf);
            // reseta o buffer
            memset(buf, 0, BUFSZ);
        }

        // fecha o socket após terminar a conexão
        close(s);

        exit(EXIT_SUCCESS);
    }
    default:
        break;
    }
}