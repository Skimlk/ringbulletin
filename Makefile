CC ?= gcc
CFLAGS ?= -Wall -Wextra -I/usr/include/libxml2/
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: ringbulletin

ringbulletin: $(OBJ)
	$(CC) $(OBJ) -o $@ -lcjson -lcurl -lxml2 -lxxhash $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f src/*.o ringbulletin
