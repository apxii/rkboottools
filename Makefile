CC      ?= gcc
#CFLAGS  ?= -O2 -Wall -Wextra
LDFLAGS ?=
PREFIX  ?= usr/local

TARGETS = rk-makebootable rk-rc4 rk-splitboot
DEPS    = Makefile

%: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

all: $(TARGETS)

install: $(TARGETS)
	install -d -m 0755 $(DESTDIR)/$(PREFIX)/bin
	install -D -m 0755 $(TARGETS) $(DESTDIR)/$(PREFIX)/bin

.PHONY: clean uninstall

clean:
	rm -f $(TARGETS)

uninstall:
	cd $(DESTDIR)/$(PREFIX)/bin && rm -f $(TARGETS)
