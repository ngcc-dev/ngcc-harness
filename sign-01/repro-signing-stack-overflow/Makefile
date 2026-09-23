CC := gcc
CFLAGS := -O1 -g -std=gnu11 -Wall -Wextra -Werror -I../../api -fno-omit-frame-pointer -fsanitize=address
CANDIDATE_CFLAGS := -O1 -g -fPIC -std=gnu11 -Wall -Wno-unused -Wno-unknown-pragmas -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer -D_GNU_SOURCE -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-int
ASAN_CANDIDATE_CFLAGS := $(CANDIDATE_CFLAGS) -fsanitize=address
LDLIBS := -ldl -fsanitize=address

.PHONY: build test clean

build: reproduce
	$(MAKE) -C ../../api harness
	$(MAKE) -B -C .. lib/libAigis-sig1.so NGCC_CFLAGS='$(CANDIDATE_CFLAGS)' NGCC_LDLIBS='-lm'
	cp ../lib/libAigis-sig1.so ../lib/libAigis-sig1-control.so
	$(MAKE) -B -C .. lib/libAigis-sig1.so NGCC_CFLAGS='$(ASAN_CANDIDATE_CFLAGS)' NGCC_LDLIBS='-lm -fsanitize=address'
	cp ../lib/libAigis-sig1.so ../lib/libAigis-sig1-asan.so

reproduce: reproduce.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

test: build
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 ./reproduce ../lib/libAigis-sig1-control.so ../lib/libAigis-sig1-asan.so

clean:
	rm -f reproduce
