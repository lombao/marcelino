VERSION=20141018-1
DIST=marcelino-$(VERSION)
SRC=marcelino.c windows.c list.c config.h events.h list.h 


CFLAGS+=-g -std=c99 -Wall -Wextra -I/usr/local/include #-DDEBUG #-DDMALLOC
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-randr -lxcb-keysyms -lxcb-icccm -lxcb-util #-ldmalloc

RM=/bin/rm
PREFIX=/usr

TARGETS=marcelino
OBJS=marcelino.o list.o windows.o conf.o mrandr.o workspace.o

all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@


marcelino.o: marcelino.c Makefile

list.o: list.c list.h Makefile

windows.o:	windows.c windows.h events.h list.h config.h Makefile

mrandr.o:	mrandr.c mrandr.h Makefile

conf.o: conf.c conf.h

workspace.o:	workspace.c workspace.h

install: $(TARGETS)
	install -m 755 marcelino $(PREFIX)/bin
	install -m 644 marcelino.man $(PREFIX)/man/man1/marcelino.1
	

uninstall: deinstall
deinstall:
	$(RM) $(PREFIX)/bin/marcelino
	$(RM) $(PREFIX)/man/man1/marcelino.1

clean:
	$(RM) -f $(TARGETS) *.o


