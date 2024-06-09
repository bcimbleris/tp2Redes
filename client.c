#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
// struct correspondente ao cliente
typedef struct {
    int socket;
    int choice;
    struct sockaddr_storage storage;
} ClientInfo;
// testa se os argumentos foram passados da maneira correta no terminal
void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server IP> <server port>\n", argv[0]);
    printf("example: %s ipv4 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024
// funcao que cria o menu principal para o cliente
void menu() {
    printf("0 - Sair\n");
    printf("1 - Senhor dos Aneis\n");
    printf("2 - O Poderoso Chefão\n");
    printf("3 - Clube da Luta\n");
}
// verifica se foram passadas as quantidades certas de argumentos
int main(int argc, char **argv) {
    if (argc < 4) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    // retorna 0 quando funciona,recebe endereço do servidor, porto e
    // ponteiro para o sockaddr_storage que vai ser inicializado
    if (0 != addrparse(argv[1], argv[2], argv[3], &storage)) {
        usage(argc, argv);
    }
    // abre socket para conexão UDP utilizando SOCK_DGRAM
    int s = socket(storage.ss_family, SOCK_DGRAM, 0);
    // checando se houve algum erro ao abrir socket
    if (s == -1) {
        logexit("socket");
    }
    // uma interface que instancia um ponteiro e é feito o cast para o tipo
    // e coloca dentro da variável
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    
    while (1) {
        //cria o objeto do tipo ClientInfo e preenche seus membros 
        ClientInfo clientInfo;
        clientInfo.socket = s;
        clientInfo.storage = storage;
        //chama a funcao de menu dentro do loop para que nao seja necessario utilizar goto e label
        menu();
        int clientResponse;
        //recebe qual opcao de filme o cliente quer
        scanf("%d", &clientResponse);

        if (clientResponse == 0) {
            break;
        }
        //membro que determina as frases que serao disponibilizadas para o cliente
        clientInfo.choice = clientResponse;
        //envia a struct correspondente ao cliente com as informações relevantes através do socket UDP inicializado anteriormente
        size_t count = sendto(s, &clientInfo, sizeof(clientInfo), 0, addr,
                              sizeof(storage));
        //checa se houve algum problema
        if (count != sizeof(clientInfo)) {
            logexit("sendto");
        }
        //comunicacao entre cliente e servidor
        char buf[BUFSZ];
        //inicializa o buffer com 0
        memset(buf, 0, BUFSZ);
        socklen_t addrlen = sizeof(storage);
        //foi utilizado um loop para contar as 5 frases que devem ser recebidas pelo cliente antes de retornar ao menu inicial
        for (int i = 0; i < 5; i++) {
            //recebe a resposta do servidor no socket s, e coloca esse dado no buff
            count = recvfrom(s, buf, BUFSZ - 1, 0, addr, &addrlen);
            if (count == -1) {
                logexit("recvfrom");
            }
            buf[count] = '\0'; // coloca ao final da recepcao da mensagem para sinalizar o final de uma frase
            //printa a frase recebida pelo servidor
            printf("Frase %d: %s\n", i + 1, buf);
        }
    }
    //fecha a conexao
    close(s);
    return 0;
}
