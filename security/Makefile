ROOT := ..
CC ?= gcc
CFLAGS ?= -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter

.PHONY: all clean
all: ngcc_security

hash_oom_false_success: hash_oom_false_success.c
	$(CC) $(CFLAGS) -o $@ hash_oom_false_success.c -ldl

ngcc_security: ngcc_security.c $(ROOT)/api/link_common.h $(ROOT)/api/link_sig.h \
	$(ROOT)/api/link_kem.h $(ROOT)/api/link_kex.h $(ROOT)/api/link_hash.h
	$(CC) $(CFLAGS) -I$(ROOT)/api -o $@ ngcc_security.c -ldl

clean:
	rm -f ngcc_security hash_oom_false_success
