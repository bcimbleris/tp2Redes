# Variáveis
CC = gcc
CFLAGS = -Wall
LDFLAGS =

# Alvos
all: bin/client bin/server-mt

# Regras de compilação
bin/client: client.o common.o | bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/server-mt: server-mt.o common.o | bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -pthread

%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para criar a pasta bin/ se ainda não existir
bin:
	@mkdir -p bin

# Regra de limpeza
clean:
	@rm -f *.o bin/client bin/server-mt
