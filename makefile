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
LIBDIR=$(PREFIX)/lib

# Install binary
INSTALL = /usr/bin/install
# C compiler
CC=gcc
# compilier flags
CFLAGS=-O
# Compiler warning
WARN=-Wall -Wno-maybe-uninitialized
# Support for locale specific character 
EUC=-DEUC
# Linker flags
LDFLAGS=
# Additionnal libraries to link with
LIBS=
# C preprocessor flags.
# Use -D_GNU_SOURCE for Linux with GNU libc.
# Use -D_INCLUDE__STDC_A1_SOURCE for HP-UX.
CPPFLAGS=-D_GNU_SOURCE
# Strip
STRIP=strip -s -R .comment -R .note

FLAGS= -DLIBDIR='"$(LIBDIR)"'

MAN1=ugrind.1
BIN=ugrind
LIB=ugrindefs

OBJ = regexp.o ugrind.o ugrindefs.o version.o ugrindroff.o

INSTALLED=$(MAN1:%=$(DESTDIR)$(MANDIR)/man1/%) \
	$(BIN:%=$(DESTDIR)$(BINDIR)/%) \
	$(LIB:%=$(DESTDIR)$(LIBDIR)/%)


all: $(BIN) $(MAN1) $(LIB)

clean:
	rm -f $(OBJ) $(BIN) $(MAN1) $(LIB)

install: $(INSTALLED)

uninstall:
	rm -f $(INSTALLED)

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(FLAGS) $(EUC) $(CPPFLAGS) -c $<

ugrind: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

ugrindefs: ugrindefs.src
	cp $< $@

%.1 %.7: %.man
	sed -e "s|@BINDIR@|$(BINDIR)|g" \
		-e "s|@LIBDIR@|$(LIBDIR)|g" $< > $@

$(DESTDIR)$(BINDIR) \
$(DESTDIR)$(LIBDIR) \
$(DESTDIR)$(MANDIR)/man1:
	test -d $@ || mkdir -p $@

$(DESTDIR)$(MANDIR)/man1/%: % $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -c -m 644 $< $@

$(DESTDIR)$(BINDIR)/%: % $(DESTDIR)$(BINDIR)
	$(INSTALL) -c $< $@

$(DESTDIR)$(LIBDIR)/%: % $(DESTDIR)$(LIBDIR)
	$(INSTALL) -c -m 644 $< $@


