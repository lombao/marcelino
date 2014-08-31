



CFLAGS+=-O2 -g -std=c99 -Wall -Wextra 
LDFLAGS+=-lxcb 	-lxcb-util -lxcb-keysyms -lxcb-icccm

RM=/usr/bin/rm
CC=/usr/bin/gcc

TARGETS=marcelino	
OBJS=marcelino.o events.o windows.o aux.o


all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	
marcelino.o: 	marcelino.c marcelino.h
events.o: 		events.c events.h
windows.o:		windows.c windows.h marcelino.h
aux.o:			aux.c aux.h

clean:	
	$(RM) -f $(TARGETS) $(OBJS)
	$(RM) -f *~
