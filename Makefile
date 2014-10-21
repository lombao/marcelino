VERSION=20141018-1
DIST=marcelino-$(VERSION)
SRC=marcelino.c windows.c list.c mrandr.c conf.c workspace.c keyboard.c events.h list.h 


CFLAGS+=-g -std=c99 -Wall -Wextra -I/usr/local/include -DDEBUG
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-randr -lxcb-keysyms -lxcb-icccm -lxcb-util 

RM=/bin/rm

PREFIX=/usr
SYSCONFDIR=/etc

TARGETS=marcelino
OBJS=marcelino.o list.o windows.o conf.o mrandr.o workspace.o keyboard.o

all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@


marcelino.o: marcelino.c Makefile

list.o: list.c list.h Makefile

windows.o:	windows.c windows.h events.h list.h conf.h Makefile

mrandr.o:	mrandr.c mrandr.h Makefile

conf.o: conf.c conf.h

workspace.o:	workspace.c workspace.h

keyboard.o:		keyboard.c keyboard.h

install: $(TARGETS)
	install -m 755 marcelino $(PREFIX)/bin
	install -m 644 marcelino.man $(PREFIX)/share/man/man1/marcelino.1
	install -m 644 marcelino.cfg $(SYSCONFDIR)

uninstall: deinstall
deinstall:
	$(RM) $(PREFIX)/bin/marcelino
	$(RM) $(PREFIX)/man/man1/marcelino.1

clean:
	$(RM) -f $(TARGETS) *.o


