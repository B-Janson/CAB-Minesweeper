CC = gcc
CFLAGS = -Wall -pedantic -pthread # Show all reasonable warnings
LDFLAGS = 

all: serverclient

serverclient: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean: 
	rm -f server client

.PHONY: all clean 
