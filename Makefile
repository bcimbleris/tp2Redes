# Variáveis
CC = gcc
CFLAGS = -Wall

# Alvos
all: bin/client bin/server

# Regras de compilação
bin/client: common.o client.c | bin
	$(CC) $(CFLAGS) client.c common.o -o $@ -lm

bin/server: common.o server.c | bin
	$(CC) $(CFLAGS) server.c common.o -o $@ -lm

common.o: common.c
	$(CC) $(CFLAGS) -c common.c

# Regra para criar a pasta bin/ se ainda não existir
bin:
	@mkdir -p bin

# Regra de limpeza
clean:
	@rm -f common.o bin/client bin/server
