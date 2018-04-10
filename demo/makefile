# Formatting text with Troff

# Where to find Heirloom tools
TROFF=/opt/utroff/bin/troff
DPOST=/opt/utroff/bin/dpost
# Where to find ugrind
UGRIND=/opt/utroff/bin/ugrind
#where to find troff fonts
FNTDIR=/opt/utroff/lib/font

TROFFONTS=$(FNTDIR)/
export TROFFONTS

all: ugrind.pdf

%.tmp: %.tr
	@echo "Generating $@"
	@$(UGRIND) $< > $@

%.to: %.tmp
	@echo "Generating $@"
	@$(TROFF) $< > $@

%.ps: %.to
	@echo "Generating $@"
	@cat $< | $(DPOST) -pa4 >$@

%.pdf: %.ps
	@echo "Generating $@"
	@ps2pdf -dPDFSETTINGS=/prepress -dEmbedAllFonts=true \
		-sFONTPATH=$(TROFFONTS) $< $@

clean:
	@rm -f ugrind.tmp ugrind.to ugrind.ps ugrind.pdf

