TARGET=bim
CFLAGS=-g -std=c99 -Wvla -pedantic -Wall -Wextra -I.

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)

THEMES=themes/ansi.o themes/citylights.o themes/solarized.o themes/sunsmoke.o themes/wombat.o
SYNTAXES=syntax/bash.o syntax/bimcmd.o syntax/biminfo.o syntax/bimrc.o syntax/c.o syntax/conf.o syntax/ctags.o \
         syntax/diff.o syntax/esh.o syntax/git.o syntax/java.o syntax/json.o syntax/make.o syntax/markdown.o \
         syntax/proto.o syntax/py.o syntax/rust.o syntax/xml.o
HEADERS=bim-core.h bim-theme.h bim-syntax.h

.PHONY: all clean distclean install install-strip uninstall

all: $(TARGET)

syntax/*.o: $(HEADERS)
themes/*.o: $(HEADERS)
*.o: $(HEADERS)

bim: bim.o $(THEMES) $(SYNTAXES)

clean:
	-rm -f $(TARGET) bim.o $(THEMES) $(SYNTAXES)

distclean: clean

install: all
	mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

.PHONY: tags
tags:
	ctags --c-kinds=+lx bim.c themes/* syntax/*
