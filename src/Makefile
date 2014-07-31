



CFLAGS+=-O2 -g -std=c99 -Wall -Wextra
LDFLAGS+=-lxcb 	-lxcb-util 

RM=/usr/bin/rm
CC=/usr/bin/gcc

TARGETS=marcelino	
OBJS=marcelino.o menu.o events.o


all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	
marcelino.o: 	marcelino.c marcelino.h
events.o: 		events.c menu.h
menu.o:			menu.c menu.h


clean:	
	$(RM) -f $(TARGETS) $(OBJS)
	$(RM) -f *~
