all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client -lm
	gcc -Wall server.c common.o -o server -lm
	gcc -Wall server-mt.c common.o -lpthread -o server-mt -lm

clean:
	rm common.o client server server-mt
