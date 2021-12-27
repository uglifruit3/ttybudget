CC = gcc
CFLAGS = -g -lm -std=gnu99 -pedantic -Wall
BIN = ttybudget

ttybudget: main.c io.c io.h backend.c backend.h
	$(CC) -o $(BIN) main.c io.c backend.c $(CFLAGS)
