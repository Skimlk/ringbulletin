CC ?= gcc
CFLAGS ?= -Wall -Wextra -I/usr/include/libxml2/
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: ringbulletin

ringbulletin: $(OBJ)
	$(CC) $(OBJ) -o $@ -lcjson -lcurl -lxml2 -lxxhash

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o ringbulletin
