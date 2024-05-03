all: client server

client: common.o client.c
	gcc -Wall client.c common.o -o bin/client -lm

server: common.o server.c
	gcc -Wall server.c common.o -o bin/server -lm

common.o: common.c
	gcc -Wall -c common.c

clean:
	rm -f common.o bin/client bin/server