TARGET=bim
CFLAGS ?=-g -O2 -std=gnu11 -Wvla -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-unused-result
LDFLAGS ?=-rdynamic

CFLAGS += $(shell docs/git-tag)

ifeq (Darwin,$(shell uname -s))
  LDLIBS=/usr/local/lib/libkuroko.a
else
  LDLIBS=-l:libkuroko.a
endif

ifeq (klange,$(shell echo $$USER))
  CFLAGS +=  -I../kuroko/src
  LDLIBS=../kuroko/libkuroko.a
endif

LDLIBS +=  -ldl -lpthread

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin
datarootdir=$(prefix)/share
datadir=$(datarootdir)

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

.PHONY: all clean distclean install install-strip uninstall

all: $(TARGET)

*.o: bim.h
bim: bim.o

clean:
	-rm -f $(TARGET) bim.o

distclean: clean

install: all
	mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)
	mkdir -p $(DESTDIR)$(datadir)/bim/themes
	$(INSTALL_DATA) themes/*.krk $(DESTDIR)$(datadir)/bim/themes/
	mkdir -p $(DESTDIR)$(datadir)/bim/syntax
	$(INSTALL_DATA) syntax/*.krk $(DESTDIR)$(datadir)/bim/syntax/
	mkdir -p $(DESTDIR)$(datadir)/bim/site
	$(INSTALL_DATA) site/*.krk $(DESTDIR)$(datadir)/bim/site/

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

.PHONY: tags
tags:
	ctags --c-kinds=+lx bim.c bim.h syntax/*
	# Action definitions create functions with the same name
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_ACTION.([a-zA-Z_]*),/\1/f/' --append bim.c
	# Command definitions are prefixed with bim_command_
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_COMMAND.([a-zA-Z_]*),/bim_command_\1/f/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_PREFIX_COMMAND.([a-zA-Z_]*),/bim_command_\1/f/' --append bim.c
	# Flexible arrays create arrays, functions, count, and space objects
	ctags --langdef=bim --language-force=bim --regex-bim='/^FLEXIBLE_ARRAY.([a-zA-Z_]*),/\1/v/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^FLEXIBLE_ARRAY.[a-zA-Z_]*,([a-zA-Z_]*),/\1/f/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^FLEXIBLE_ARRAY.([a-zA-Z_]*),/flex_\1_count/v/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^FLEXIBLE_ARRAY.([a-zA-Z_]*),/flex_\1_space/v/' --append bim.c
	
