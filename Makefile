TARGET=bim
CFLAGS=-g -std=c99 -Wvla -pedantic -Wall -Wextra

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)

.PHONY: all clean distclean install install-strip uninstall

all: $(TARGET)

clean:
	-rm -f $(TARGET)

distclean: clean

install: all
	mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(TARGET)

tags: bim.c
	ctags --c-kinds=+lx bim.c
