CFLAGS = -O2 -pipe -std=c23 -Wall -Werror -march=native -fno-semantic-interposition
CXXFLAGS="${CFLAGS}"


LIBDIR = /usr/lib
#LDFLAGS = -nostartfiles

LIBS = -lncurses -lsensors

CC = gcc

sensible:
	$(CC) $(CFLAGS) $(LIBS) -o sensible main.c

clean:
	rm -r sensible
