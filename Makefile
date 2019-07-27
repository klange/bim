TARGET=bim
CFLAGS=-g -std=c99 -Wvla -pedantic -Wall -Wextra -Werror
SOURCES=src/bim.c  src/biminfo.c  src/buffer.c  src/history.c  src/line.c  src/syntax.c  src/themes.c  src/utf8.c  src/util.c  src/terminal.c  src/display.c  src/navigation.c  src/config.c  src/search.c
OBJECTS=src/bim.o  src/biminfo.o  src/buffer.o  src/history.o  src/line.o  src/syntax.o  src/themes.o  src/utf8.o  src/util.o  src/terminal.o  src/display.o  src/navigation.o  src/config.o  src/search.o

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)

.PHONY: all clean distclean install install-strip uninstall

all: $(TARGET)

bim: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

clean:
	-rm -f $(TARGET)
	-rm -f $(OBJECTS)

distclean: clean

install: all
	mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

tags: $(SOURCES)
	ctags --c-kinds=+lx bim.c
