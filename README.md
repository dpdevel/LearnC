# LearnC
Tutorial to C Programming

Compile with Makefile:
```
CC := gcc
CFLAGS := -Wall -Wextra -O2 -pedantic -Wpedantic -Wformat -pthread
.PHONY: all clean help

clean:
        rm -f *.G *~ $(PROGS)
help:
        @echo " USage ..."
```

or use gcc command:
```
$ gcc -Wall -Wextra -O2 -pedantic -Wpedantic -Wformat -pthread <filename.c> -o <filename>
```
