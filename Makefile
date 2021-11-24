CC = gcc
CFLAGS = -g -lm -std=c99 -pedantic -Wall

test: main.c io.c backend.c
	$(CC) -o test main.c io.c backend.c $(CFLAGS)
