.PHONY: test build clean

CC := gcc
CFLAGS := -O2 -std=gnu11 -Wall -Wextra -Werror -fsanitize=address -fno-omit-frame-pointer
CPPFLAGS := -I../../api
LDLIBS := -ldl -fsanitize=address
NGCC_FIXED_CFLAGS := -O2 -fPIC -std=gnu11 -Wall -Wno-unused -Wno-unknown-pragmas -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer -D_GNU_SOURCE
NGCC_FIXED_LDFLAGS := -shared -Wl,--version-script=../api/link_exports.map -Wl,-Bsymbolic -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro

reproduce: reproduce.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDLIBS)

build: reproduce
	$(MAKE) -C .. -B lib/libAigis-sig1.so \
		NGCC_CFLAGS='$(NGCC_FIXED_CFLAGS)' \
		NGCC_LDFLAGS='$(NGCC_FIXED_LDFLAGS)' \
		NGCC_LDLIBS='-lm'

test: build
	./reproduce ../lib/libAigis-sig1.so

clean:
	rm -f reproduce
