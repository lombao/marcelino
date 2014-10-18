VERSION=20130209-2
DIST=marcelino-$(VERSION)
SRC=marcelino.c list.c config.h events.h list.h hidden.c
DISTFILES=LICENSE Makefile NEWS README TODO WISHLIST marcelino.man hidden.man scripts $(SRC)

CFLAGS+=-g -std=c99 -Wall -Wextra -I/usr/local/include #-DDEBUG #-DDMALLOC
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-randr -lxcb-keysyms -lxcb-icccm \
	-lxcb-util #-ldmalloc

RM=/bin/rm
PREFIX=/usr/local

TARGETS=marcelino hidden
OBJS=marcelino.o list.o

all: $(TARGETS)

marcelino: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

hidden: hidden.c
	$(CC) $(CFLAGS) hidden.c $(LDFLAGS) -o $@

marcelino-static: $(OBJS)
	$(CC) $(OBJS) -static $(CFLAGS) $(LDFLAGS) \
	-lXau -lXdmcp -o $@

marcelino.o: marcelino.c events.h list.h config.h Makefile

list.o: list.c list.h Makefile

install: $(TARGETS)
	install -m 755 marcelino $(PREFIX)/bin
	install -m 644 marcelino.man $(PREFIX)/man/man1/marcelino.1
	install -m 755 hidden $(PREFIX)/bin
	install -m 644 hidden.man $(PREFIX)/man/man1/hidden.1

uninstall: deinstall
deinstall:
	$(RM) $(PREFIX)/bin/marcelino
	$(RM) $(PREFIX)/man/man1/marcelino.1
	$(RM) $(PREFIX)/bin/hidden
	$(RM) $(PREFIX)/man/man1/hidden.1

$(DIST).tar.bz2:
	mkdir $(DIST)
	cp -r $(DISTFILES) $(DIST)/
	tar cf $(DIST).tar --exclude .git $(DIST)
	bzip2 -9 $(DIST).tar
	$(RM) -rf $(DIST)

dist: $(DIST).tar.bz2

clean:
	$(RM) -f $(TARGETS) *.o

distclean: clean
	$(RM) -f $(DIST).tar.bz2
