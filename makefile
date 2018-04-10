# Packaging directory
DESTDIR=
# Prefix directory
PREFIX=/opt/utroff
# Where to place binaries
# and where to find utroff tools
BINDIR=$(PREFIX)/bin
# Where to place manuals
MANDIR=$(PREFIX)/man
# Where to place libraries
LIBDIR=$(PREFIX)/lib/

# C compiler
CC=cc
# compilier flags
CFLAGS=-Wall -Wno-maybe-uninitialized -O
FLAGS= -DLIBDIR='"$(LIBDIR)"'
# Linker flags
LDFLAGS=


all: ugrind ugrind.1 ugrindefs

OBJ = regexp.o ugrind.o ugrindefs.o version.o ugrindroff.o
.c.o:
	$(CC) $(CFLAGS) $(WARN) $(FLAGS) -c $<

ugrind: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

ugrindefs: ugrindefs.src
	cp $< $@

%.1: %.man
	sed -e "s|@BINDIR@|$(BINDIR)|g" \
		-e "s|@LIBDIR@|$(LIBDIR)|g" $< > $@

clean:
	rm -f $(OBJ) ugrind ugrind.1 ugrindefs

INSTALLED=$(DESTDIR)$(MANDIR)/man1/ugrind.1 \
		  $(DESTDIR)$(BINDIR)/ugrind \
		  $(DESTDIR)$(LIBDIR)/ugrindefs

$(DESTDIR)$(MANDIR)/man1/%: %
	test -d $(DESTDIR)$(MANDIR) || mkdir -p $(DESTDIR)$(MANDIR)
	install -c -m 644 $< $@

$(DESTDIR)$(BINDIR)/%: % $(DESTDIR)$(BINDIR)
	test -d $(DESTDIR)$(BINDIR) || mkdir -p $(DESTDIR)$(BINDIR)
	install -c $< $@

$(DESTDIR)$(LIBDIR)/%: %
	test -d $(DESTDIR)$(LIBDIR) || mkdir -p $(DESTDIR)$(LIBDIR)
	install -c -m 644 $< $@

install: $(INSTALLED)

uninstall:
	rm -f $(INSTALLED)


