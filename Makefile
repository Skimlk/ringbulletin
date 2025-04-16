CC?=gcc

all: ringbulletin clean

ringbulletin: main.o fetch.o
	$(CC) main.o fetch.o -o ringbulletin -lcurl -lxml2 -I/usr/include/libxml2/

main.o: main.c
	$(CC) -c main.c

fetch.o: fetch.c fetch.h
	$(CC) -c fetch.c

clean:
	rm -f *.o 
