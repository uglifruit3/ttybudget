CC = gcc
CFLAGS = -g -lm -std=gnu99 -Wpedantic -Wall
DEPS = io.h backend.h
OBJS = main.o io.o backend.o
BIN = ttybudget
INSTALLPATH = /usr/bin/
MANPATH = /usr/local/share/man/man1
USR = $(USER)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
ttybudget: $(OBJS)
	$(CC) -o $(BIN) $^ $(CFLAGS)

clean:
	# get rid of loose object files
	rm -f *.o

install: $(BIN)
	# install binary
	mv $(BIN) $(INSTALLPATH)
	# install manpage
	if [ ! -d $(MANPATH) ]; then mkdir --parents $(MANPATH); fi
	cp ttybudget.1.gz $(MANPATH)/
	# create the initial records location if nonexistent
	if [ ! -d /home/$(USR)/.local/share/ttybudget ]; then mkdir --parents /home/$(USR)/.local/share/ttybudget; fi
	chown $(USR) /home/$(USR)/.local/share/ttybudget/
	# create the config file if nonexistent
	if [ ! -d /home/$(USR)/.config/ttybudget ]; then mkdir --parents /home/$(USR)/.config/ttybudget; fi
	if [ ! -f /home/$(USR)/.config/ttybudget/defaults.conf ]; then cp template.defaults.conf /home/$(USR)/.config/ttybudget/defaults.conf; fi
	chown $(USR) /home/$(USR)/.config/ttybudget/defaults.conf
