

SRC=marcelino.c

CFLAGS+=-O2 -g -std=c99 -Wall -Wextra
LDFLAGS+=-lxcb 	-lxcb-util 

RM=/usr/bin/rm
PREFIX=/usr/local

CC=gcc

TARGETS=marcelino	
OBJS=marcelino.o menu.o events.o


all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	
marcelino.o: 	src/marcelino.c src/marcelino.h
events.o: 		src/events.c 
menu.o:			src/menu.c


clean:	
	$(RM) -f $(TARGETS) *.o
	$(RM) -f *~
	$(RM) -f src/*~
