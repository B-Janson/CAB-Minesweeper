CC = gcc
CFLAGS = -std=c99 -Wall -pedantic # Show all reasonable warnings
SFLAGS = -pthread
LDFLAGS = 

all: serverclient

serverclient: server client

server: server.c
	$(CC) $(CFLAGS) ${SFLAGS} -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean: 
	rm -f server client

.PHONY: all clean 
