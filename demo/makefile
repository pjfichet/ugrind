PREFIX=$(HOME)/.local
BINDIR=$(PREFIX)/bin
ROFF=$(BINDIR)/roff
POST=$(BINDIR)/pdf
UGRIND=$(BINDIR)/ugrind

all: ugrind.pdf

%.pdf: %.tr
	@$(UGRIND) $< | $(ROFF) -mus | $(POST) | ps2pdf - >$@

clean:
	@rm -f ugrind.pdf

