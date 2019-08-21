TARGET=bim
CFLAGS=-g -flto -std=c99 -Wvla -pedantic -Wall -Wextra -I. $(shell docs/git-tag)

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)

THEMES = $(patsubst %.c, %.o, $(wildcard themes/*.c))
SYNTAXES = $(patsubst %.c, %.o, $(wildcard syntax/*.c))
HEADERS = $(wildcard bim-*.h)

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
	ctags --c-kinds=+lx bim.c bim-*.h themes/* syntax/*
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_ACTION.([a-zA-Z_]*),/\1/f/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_COMMAND.([a-zA-Z_]*),/bim_command_\1/f/' --append bim.c
	ctags --langdef=bim --language-force=bim --regex-bim='/^BIM_PREFIX_COMMAND.([a-zA-Z_]*),/bim_command_\1/f/' --append bim.c
