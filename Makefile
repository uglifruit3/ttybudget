CC = gcc
CFLAGS = -g -lm -std=gnu99 -Wpedantic -Wall
DEPS = io.h backend.h
OBJS = main.o io.o backend.o
BIN = ttybudget
INSTALLPATH = /usr/bin/
USR = $(USER)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
ttybudget: $(OBJS)
	$(CC) -o $(BIN) $^ $(CFLAGS)
	# create the initial records location if nonexistent
	if [ ! -d /home/$(USR)/.local ]; then mkdir /home/$(USR)/.local; fi
	if [ ! -d /home/$(USR)/.local/share ]; then mkdir /home/$(USR)/.local/share; fi
	if [ ! -d /home/$(USR)/.local/share/ttybudget ]; then mkdir /home/$(USR)/.local/share/ttybudget; fi
	# create the config file if nonexistent
	if [ ! -d /home/$(USR)/.config ]; then mkdir /home/$(USR)/.config; fi
	if [ ! -d /home/$(USR)/.config/ttybudget ]; then mkdir /home/$(USR)/.config/ttybudget; fi
	if [ ! -f /home/$(USR)/.config/ttybudget/defaults.conf ]; then cp template.defaults.conf /home/$(USR)/.config/ttybudget/defaults.conf; fi
	# get rid of loose object files
	rm -f *.o

install: $(BIN)
	if [ -f $(INSTALLPATH)$(BIN) ]; then rm $(INSTALLPATH)$(BIN); fi
	mv $(BIN) $(INSTALLPATH)
