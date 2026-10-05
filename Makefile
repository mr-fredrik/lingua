CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 

PREFIX = ~/.local
BINDIR = $(PREFIX)/bin
SHAREDIR = $(PREFIX)/share/lingua

.PHONY: all clean install uninstall

all: lingua

lingua: lingua.c
	$(CC) $(CFLAGS) -o lingua lingua.c $(LDFLAGS)

clean:
	rm -f lingua

install: lingua
	mkdir -p $(BINDIR)
	mkdir -p $(SHAREDIR)
	cp lingua $(BINDIR)/lingua
	cp twisters.txt $(SHAREDIR)/twisters.txt
	cp jokes.txt $(SHAREDIR)/jokes.txt
	@echo "Installed to $(BINDIR)/lingua and $(SHAREDIR)"

uninstall:
	rm -f $(BINDIR)/lingua
	rm -f $(SHAREDIR)/twisters.txt
	rm -f $(SHAREDIR)/jokes.txt
	rmdir $(SHAREDIR) 2>/dev/null || true
