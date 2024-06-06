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
#define PI 3.14159265358979323846

typedef struct {
    double latitude;
    double longitude;
} Coordinate;

void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server port>\n", argv[0]);
    printf("example: %s ipv4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Função para calcular a distância entre duas coordenadas usando a fórmula de Haversine
double deg2rad(double deg) { return deg * (PI / 180.0); }

double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
    lat1 = deg2rad(lat1);
    lon1 = deg2rad(lon1);
    lat2 = deg2rad(lat2);
    lon2 = deg2rad(lon2);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = EARTH_RADIUS * c;

    return distance;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    Coordinate coordServ = {-19.9227, -43.9451};

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
    printf("Aguardando solicitação.\n");

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);

        Coordinate coordCli;
        ssize_t count = recvfrom(s, &coordCli, sizeof(Coordinate), 0, caddr, &caddrlen);
        if (count != sizeof(Coordinate)) {
            logexit("recvfrom");
        }

        printf("Corrida disponível:\n");
        printf("0 - Recusar\n");
        printf("1 - Aceitar\n");
        int sResponse;
        scanf("%d", &sResponse);

        switch (sResponse) {
            case 0: {
                char *message1 = "Não foi encontrado um motorista";
                sendto(s, message1, strlen(message1), 0, caddr, caddrlen);
                printf("Aguardando solicitação.\n");
                break;
            }
            case 1: {
                double distance = haversine_distance(coordCli.latitude, coordCli.longitude,
                                                     coordServ.latitude, coordServ.longitude);

                while (distance > 0) {
                    char message[BUFSZ];
                    sprintf(message, "Distância do motorista: %.2f km\n", distance);
                    sendto(s, message, strlen(message), 0, caddr, caddrlen);
                    usleep(2000000); // Aguarda 2 segundos
                    distance -= 0.4; // Reduz a distância em 400m a cada 2 segundos
                }

                char *arrival_message = "O motorista chegou!";
                sendto(s, arrival_message, strlen(arrival_message), 0, caddr, caddrlen);
                printf("O motorista chegou!\n");
                printf("Aguardando solicitação.\n");
                break;
            }
            default:
                printf("Opção inválida. Tente novamente.\n");
                break;
        }
    }

    close(s);
    exit(EXIT_SUCCESS);
}
