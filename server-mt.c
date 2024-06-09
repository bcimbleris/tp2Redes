#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024
// funcao que verifica se o programa foi utilizado de maneira correta
void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server port>\n", argv[0]);
    printf("example: %s ipv4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}
// cria a struct para os dados do clietne
typedef struct {
    int socket;
    int choice;
    struct sockaddr_storage storage;
} ClientInfo;
// abaixo foram inicializados arrays com as frases dos filmes escolhidos
const char *lotr_quotes[] = {
    "Um anel para a todos governar",
    "Na terra de Mordor onde as sombras se deitam "
    "Não é o que temos, mas o que fazemos com o que temos ",
    "Não há mal que sempre dure", "O mundo está mudando, senhor Frodo"};

const char *godfather_quotes[] = {
    "Vou fazer uma oferta que ele não pode recusar",
    "Mantenha seus amigos por perto e seus inimigos mais perto ainda",
    "É melhor ser temido que amado", "A vingança é um prato que se come frio",
    "Nunca deixe que ninguém saiba o que você está pensando"};

const char *fightclub_quotes[] = {
    "Primeira regra do Clube da Luta: você não fala sobre o Clube da Luta ",
    "Segunda regra do Clube da Luta : você não fala sobre o Clube da Luta",
    " O que você possui acabará possuindo você ",
    "É apenas depois de perder tudo que somos livres para fazer qualquer coisa",
    " Escolha suas lutas com sabedoria "};

// uma variavel mutex foi a melhor maneira encontrada atraves de pesquisa para
// conseguir mostrar a quantidade de clientes ativos a cada 4 segundos.
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;
void *monitor_clients(void *arg) {
    while (1) {
        sleep(4);
        // utiliza-se uma funcao que previne que outras threads atrapalhem a
        // contagem de clientes ativos em um determinado instante. Essa funcao
        // trava a quantidade de clientes antes de mostrar
        pthread_mutex_lock(&client_count_mutex);
        printf("Clientes: %d\n", client_count);
        // essa funcao libera a contagem para mudar
        pthread_mutex_unlock(&client_count_mutex);
    }
    return NULL;
}
// funcao responsavel por enviar as frases para o cliente
void send_quotes(int sock, const char *quotes[], struct sockaddr *caddr,
                 socklen_t caddrlen) {
    // buffer para o envio de informacoes para o cliente
    char send_buf[BUFSZ];
    for (int i = 0; i < 5; i++) {
        // escreve no buffer as frases dos filmes
        snprintf(send_buf, BUFSZ, "%s\n", quotes[i]);
        // envia para o cliente atraves do socket a frase correspondente à sua
        // escolha de filme
        size_t send_count =
            sendto(sock, send_buf, strlen(send_buf), 0, caddr, caddrlen);
        if (send_count != strlen(send_buf)) {
            logexit("sendto");
        }
        sleep(3);
    }
}
// inicializa a thread para que possam ser possiveis varios clientes
// simultaneamente
void *client_thread(void *data) {
    ClientInfo *clientInfo = (ClientInfo *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&clientInfo->storage);
    socklen_t caddrlen = sizeof(clientInfo->storage);

    pthread_mutex_lock(&client_count_mutex);
    // incrementa a contagem de clientes ativos
    client_count++;
    pthread_mutex_unlock(&client_count_mutex);
    // chama a funcao send_quotes a partir da escolha feita pelo cliente
    if (clientInfo->choice == 1) {
        send_quotes(clientInfo->socket, lotr_quotes, caddr, caddrlen);
    } else if (clientInfo->choice == 2) {
        send_quotes(clientInfo->socket, godfather_quotes, caddr, caddrlen);
    } else if (clientInfo->choice == 3) {
        send_quotes(clientInfo->socket, fightclub_quotes, caddr, caddrlen);
    }

    pthread_mutex_lock(&client_count_mutex);
    // decrementa a contagem de clientes ativos
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);
    // libera a memoria relativa as informacoes do cliente apos finalizacao do
    // envio e finaliza a thread
    free(clientInfo);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    // checa se o programa esta sendo utilizado corretamente
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }
    // inicializa o socket UDP
    int s = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    // verifica se houve algum probelma com o socket
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // realiza o bind de um nome para um socket
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    // Declara uma variável do tipo pthread_t para identificar a thread de
    // monitoramento.
    pthread_t monitor_tid;
    // Cria uma nova thread de monitoramento usando pthread_create, utiliza o
    // endereço da variável monitor_tid onde o ID da nova thread será
    // armazenado, função que a nova thread irá executar e flag que nao e
    // utilizada
    pthread_create(&monitor_tid, NULL, monitor_clients, NULL);
    // Desanexa a nova thread de monitoramento da thread principal, permite que
    // a nova thread continue a execução independentemente da thread principal.
    pthread_detach(monitor_tid);
    // o loop abaixo aguarda a chegada de mensagens do cliente
    while (1) {
        // aloca memoria
        ClientInfo *clientInfo = malloc(sizeof(*clientInfo));
        // testa se a alocacao de memoria foi bem sucedida
        if (!clientInfo) {
            logexit("malloc");
        }
        // tamanho do endereco do cliente
        socklen_t caddrlen = sizeof(clientInfo->storage);
        // configura o socket
        clientInfo->socket = s;
        // recebe os dados do cliente
        int recv_count =
            recvfrom(s, clientInfo, sizeof(*clientInfo), 0,
                     (struct sockaddr *)(&clientInfo->storage), &caddrlen);
        if (recv_count == -1) {
            logexit("recvfrom");
        }
        // Cria uma nova thread para lidar com as informações recebidas do
        // cliente.
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, clientInfo);
        // Desanexa a nova thread para liberar seus recursos automaticamente
        // após a conclusão.
        pthread_detach(tid);
    }
    // fecha o socket
    close(s);
    return 0;
}
