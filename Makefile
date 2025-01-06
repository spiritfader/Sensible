CFLAGS = -std=c23 -Wall -Werror

LIBDIR = /usr/lib
#LDFLAGS = -nostartfiles

LIBS = -lncurses -lsensors

CC = gcc 

sensible:
	$(CC) $(CFLAGS) $(LIBS) -o sensible main.c
