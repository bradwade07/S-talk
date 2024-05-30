
CFLAGS = -Wall -Wpedantic -ggdb -std=c99 -D _POSIX_C_SOURCE=200809L

all: build

build:
	gcc $(CFLAGS) s-talk.c threads.c list.o -lpthread -o stalk

clean:
	rm -f stalk new_core*