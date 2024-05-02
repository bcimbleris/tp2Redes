#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
// quando dá erro imprime a mensagem e o código de erro
void logexit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
// se recebe endereço nulo  ou port nulo fecha o main
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1;
    }
    // padrao do protocolo da internet é 16 bits. faz cast de string para
    // inteiro
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    // foi passado um número errado, então finaliza o programa
    if (port == 0) {
        return -1;
    }
    // o numero do porto precisa estar em network byte order. Coloca o port em
    // big endian, que é a representação correta
    port = htons(port); // host to network short

    // checa se é IPv4, se der erro tenta se é IPv6, se der erro de novo
    // finaliza o programa
    struct in_addr inaddr4; // 32-bit IP address
    // tenta fazer o parser de IPv4 e se der certo coloca no inaddr4
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        // converte a struct para um sockaddr_in, que é o sockadrr da internet
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IPv6 address
                             // tenta fazer o parser de IPv6 e se der certo
                             // coloca no inaddr6
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6
        // para poder pegar 16 bytes e copiar para outro lugar precisa ser feito
        // dessa maneira em C
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}
// recebe um sockaddr como parametro  o string onde vai ser printado e o tamanho
// do string
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    // utiliza-se esse pois pode ser IPv4 ou IPv6
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;
    // verifica qual é a familia do protocolo, IPv4 ou IPv6
    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        // converte de representação de rede para representação textual
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        // converte de representação de rede para representação de dispositivo
        port = ntohs(addr4->sin_port); // network to host short
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        // converte de representação de rede para representação textual
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        // converte de representação de rede para representação de dispositivo
        port = ntohs(addr6->sin6_port); // network to host short
    } else {
        logexit("unknown protocol family.");
    }
    // printa o que foi passado
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}
// faz o parse da porta
int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    // checa qual protocolo o cliente mandou o servidor rodar
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        // passa qualquer endereço que o computador tenha na interface de redes
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        // passa qualquer endereço que o computador tenha na interface de redes
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}
