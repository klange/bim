.PHONY: $(TARGET) all clean install uninstall
CFLAGS = -std=c99 -g -D__BSD_VISIBLE
TARGET = bim
DESTDIR ?= /usr/bin

$(TARGET):

all: $(TARGET)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) "${DESTDIR}"

uninstall:
	rm "${DESTDIR}"/$(TARGET)
