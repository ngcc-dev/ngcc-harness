CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic
LDLIBS ?= -ldl -lcrypto
.PHONY: all harness clean
all: harness
harness: ../bin/nist-sig
../bin/nist-sig: nist_sig.c nist_sig_api.h
	mkdir -p ../bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ nist_sig.c $(LDFLAGS) $(LDLIBS)
clean:
	rm -f ../bin/nist-sig
