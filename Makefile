

SRC=marcelino.c

CFLAGS+=-O2 -g -std=c99 -Wall -Wextra
LDFLAGS+=-lxcb 	-lxcb-util 

RM=/usr/bin/rm
PREFIX=/usr/local

CC=gcc

TARGETS=marcelino	



all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) src/marcelino.c
	


clean:	
	$(RM) -f $(TARGETS) *.o
	$(RM) -f *~
	$(RM) -f src/*~
