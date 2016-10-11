CC = gcc -Wall
LDFLAGS = -lpthread

all: server client run

server: server.c utility.h
	$(CC) -o server server.c $(LDFLAGS)

client: client.c utility.h
	$(CC) -o client client.c $(LDFLAGS)

run:
	./server

.PHONY: clean

clean:
	rm -f client server
