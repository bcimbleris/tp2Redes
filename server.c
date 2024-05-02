#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#define BUFSZ 1024
#define EARTH_RADIUS 6371.0 // Raio da Terra em quilômetros
typedef struct {
    double latitude;
    double longitude;
} Coordinate;

// recebe o tipo de protocolo do servidor e o porto onde ele vai ficar esperando
// e um exemplo
void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}
// Função para calcular a distância entre duas coordenadas usando a fórmula de
// Haversine
double deg2rad(double deg) {
    return deg * (M_PI / 180.0);
}
double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
    // Converte as coordenadas de graus para radianos
    lat1 = deg2rad(lat1);
    lon1 = deg2rad(lon1);
    lat2 = deg2rad(lat2);
    lon2 = deg2rad(lon2);

    // Calcula as diferenças de latitude e longitude
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Aplica a fórmula de Haversine
    double a =
        pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = EARTH_RADIUS * c;

    return distance;
}
int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }
    // criando uma coordenada
    Coordinate coordServ = {-19.9927, -43.9451};

    struct sockaddr_storage storage;
    // recebe o tipo o porto e o storage para inicializar
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    // inicializa o socket com o tipo do protocolo
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }
    // passa uma opção para o socket s reutilizar o porto caso o porto ja
    // estiver sendo utilizado
    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // quando o bind da certo retorna 0
    // o bind recebe o socket, a estrutura e o tamanho dela
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }
    // quando o listen da certo retorna 0
    // passa o socket e a quantidade de conexões que podem estar pendentes para
    // tratamento
    if (0 != listen(s, 10)) {
        logexit("listen");
    }
    // imprime uma mensagem para falar que o bind deu certo e que o servidor
    // está esperando conexões
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Aguardando solicitação.\n");
    // é feito um loop infinito para tratar as conexões
    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        // para passar para o accept da maneira correta
        socklen_t caddrlen = sizeof(cstorage);
        // retorna um novo socket para falar com o cliente e armazena o endereço
        // do cliente em caddr
        int csock = accept(s, caddr, &caddrlen);

        if (0 == listen(s, 10)) {

            printf("Corrida disponível:\n ");
            printf("0 - Recusar\n ");
            printf("1 - Aceitar\n ");
            int sResponse;
            scanf("%d", &sResponse);
            switch (sResponse) {
            case 0: {

                char *message = "Não foi encontrado um motorista";
                send(csock, message, strlen(message), 0);
                close(csock);
                printf("Aguardando solicitação.\n");
                break;
            }
            case 1: {

                // verifica se a conexão foi bem sucedida ou não
                if (csock == -1) {
                    logexit("accept");
                }
                // printa o endereço do cliente colocado em caddr
                char caddrstr[BUFSZ];
                addrtostr(caddr, caddrstr, BUFSZ);
                // printf("[log] connection from %s\n", caddrstr);
                //  le a mensagem enviada pelo cliente
                char buf[BUFSZ];
                // zera a memoria para não printar informações indesejadas
                memset(buf, 0, BUFSZ);
                // recebe a mensagem
                Coordinate coordCli;
                size_t count = recv(csock, &coordCli, sizeof(Coordinate), 0);
                // printa a mensagem do cliente e o tamanho dela
                printf("Coordenadas recebidas: Latitude = %f, Longitude = %f\n",
                       coordCli.latitude, coordCli.longitude);
                // Calcular a distância entre as coordenadas do cliente e do
                // server
                double distance =
                    haversine_distance(coordCli.latitude, coordCli.longitude,
                                       coordServ.latitude, coordServ.longitude);
                printf("Distância entre cliente e serviço: %.2f km\n",
                       distance);
                // printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count,
                // buf);
                //  envia a resposta para o cliente em cima do buf. limita para
                //  printar ate 1000 bytes e nao dar buffer overflow
                // sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
                //  envia no csock a mensagem
                count = send(csock, buf, strlen(buf) + 1, 0);
                // caso não tenha sido enviado o numero certo de dados fecha o
                // programa
                if (count != strlen(buf) + 1) {
                    logexit("send");
                }
                // fecha a conexao
                printf("Aguardando solicitação.\n");
                close(csock);
            }
            default:
                break;
            }
        }
    }

    exit(EXIT_SUCCESS);
}