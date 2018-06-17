CC := gcc
CFLAGS := -Wall -Wextra -O2 -pedantic -Wpedantic -Wformat -pthread

PROGD = hello \
	

.PHONY: all clean help

clean:
	rm -f *.G *~ $(PROGS)

help:
	@echo " USage ..."
