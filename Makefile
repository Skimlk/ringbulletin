CC?=gcc

all: ringbulletin clean

ringbulletin: main.o loadjson.o fetch.o
	$(CC) main.o loadjson.o fetch.o -o ringbulletin -lcjson -lcurl -lxml2 -I/usr/include/libxml2/

main.o: main.c
	$(CC) -c main.c

loadjson.o: loadjson.c loadjson.h
	$(CC) -c loadjson.c

fetch.o: fetch.c fetch.h
	$(CC) -c fetch.c

clean:
	rm -f *.o 
