CC = gcc
CFLAGS = -g -lm -std=gnu99 -Wpedantic -Wall
DEPS = io.h backend.h
OBJS = main.o io.o backend.o
BIN = ttybudget
INSTALLPATH = /usr/bin/

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
ttybudget: $(OBJS)
	$(CC) -o $(BIN) $^ $(CFLAGS)

clean:
	rm -f *.o

install: $(BIN)
	if [ -f $(INSTALLPATH)$(BIN) ]; then rm $(INSTALLPATH)$(BIN); fi
	mv $(BIN) $(INSTALLPATH)

records:
	if [ ! -d ~/.local ]; then mkdir ~/.local; fi
	if [ ! -d ~/.local/share ]; then mkdir ~/.local/share; fi
	if [ ! -d ~/.local/share/ttybudget ]; then mkdir ~/.local/share/ttybudget; fi
