# Local explicit-source shared-library build. Submitted build scripts are not executed.
CC ?= cc
CFLAGS ?= -O2 -g
ROOT := ..
HEADERS := $(shell rg --files submission -g '*.h' -g '*.inc' -g '*.macros' -g '*.inl')
.PHONY: all libs clean
libs: all
all: lib/mayo_1.so lib/mayo_2.so lib/mayo_3.so lib/mayo_5.so

lib/mayo_1.so: submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_1/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile $(HEADERS)
	mkdir -p lib
	$(CC) $(CFLAGS) -std=gnu11 -fPIC -shared -Wl,-z,defs -I../api -DNIST_SIG_SCHEME='"MAYO"' -DNIST_SIG_PARAMETER='"mayo_1"' -DMAYO_BUILD_TYPE_REF -DMAYO_VARIANT=MAYO_1 -DNIST_SIG_LENGTH_TYPE=size_t -Isubmission/MAYO/Reference_Implementation/include -Isubmission/MAYO/Reference_Implementation/src -Isubmission/MAYO/Reference_Implementation/src/common -Isubmission/MAYO/Reference_Implementation/src/generic -Isubmission/MAYO/Reference_Implementation/src/mayo_1 submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_1/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c -o $@ $(LDFLAGS)

lib/mayo_2.so: submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_2/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile $(HEADERS)
	mkdir -p lib
	$(CC) $(CFLAGS) -std=gnu11 -fPIC -shared -Wl,-z,defs -I../api -DNIST_SIG_SCHEME='"MAYO"' -DNIST_SIG_PARAMETER='"mayo_2"' -DMAYO_BUILD_TYPE_REF -DMAYO_VARIANT=MAYO_2 -DNIST_SIG_LENGTH_TYPE=size_t -Isubmission/MAYO/Reference_Implementation/include -Isubmission/MAYO/Reference_Implementation/src -Isubmission/MAYO/Reference_Implementation/src/common -Isubmission/MAYO/Reference_Implementation/src/generic -Isubmission/MAYO/Reference_Implementation/src/mayo_2 submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_2/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c -o $@ $(LDFLAGS)

lib/mayo_3.so: submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_3/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile $(HEADERS)
	mkdir -p lib
	$(CC) $(CFLAGS) -std=gnu11 -fPIC -shared -Wl,-z,defs -I../api -DNIST_SIG_SCHEME='"MAYO"' -DNIST_SIG_PARAMETER='"mayo_3"' -DMAYO_BUILD_TYPE_REF -DMAYO_VARIANT=MAYO_3 -DNIST_SIG_LENGTH_TYPE=size_t -Isubmission/MAYO/Reference_Implementation/include -Isubmission/MAYO/Reference_Implementation/src -Isubmission/MAYO/Reference_Implementation/src/common -Isubmission/MAYO/Reference_Implementation/src/generic -Isubmission/MAYO/Reference_Implementation/src/mayo_3 submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_3/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c -o $@ $(LDFLAGS)

lib/mayo_5.so: submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_5/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile $(HEADERS)
	mkdir -p lib
	$(CC) $(CFLAGS) -std=gnu11 -fPIC -shared -Wl,-z,defs -I../api -DNIST_SIG_SCHEME='"MAYO"' -DNIST_SIG_PARAMETER='"mayo_5"' -DMAYO_BUILD_TYPE_REF -DMAYO_VARIANT=MAYO_5 -DNIST_SIG_LENGTH_TYPE=size_t -Isubmission/MAYO/Reference_Implementation/include -Isubmission/MAYO/Reference_Implementation/src -Isubmission/MAYO/Reference_Implementation/src/common -Isubmission/MAYO/Reference_Implementation/src/generic -Isubmission/MAYO/Reference_Implementation/src/mayo_5 submission/MAYO/Reference_Implementation/src/mayo.c submission/MAYO/Reference_Implementation/src/params.c submission/MAYO/Reference_Implementation/src/arithmetic.c submission/MAYO/Reference_Implementation/src/mayo_5/api.c submission/MAYO/Reference_Implementation/src/common/randombytes_ctrdrbg.c submission/MAYO/Reference_Implementation/src/common/aes_c.c submission/MAYO/Reference_Implementation/src/common/aes128ctr.c submission/MAYO/Reference_Implementation/src/common/aes_neon.c submission/MAYO/Reference_Implementation/src/common/fips202.c submission/MAYO/Reference_Implementation/src/common/mem.c ../api/nist_sig_shim.c -o $@ $(LDFLAGS)

clean:
	rm -rf lib results

KAT_LIMIT ?= 0
.PHONY: test test-mayo_1 test-mayo_2 test-mayo_3 test-mayo_5
test: test-mayo_1 test-mayo_2 test-mayo_3 test-mayo_5
	python3 ../tools/run_library_kat.py --scheme MAYO --check-summary --limit $(KAT_LIMIT)

test-mayo_1: lib/mayo_1.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme MAYO --parameter mayo_1 --library lib/mayo_1.so --manifest kat/mayo_1.tsv --limit $(KAT_LIMIT)

test-mayo_2: lib/mayo_2.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme MAYO --parameter mayo_2 --library lib/mayo_2.so --manifest kat/mayo_2.tsv --limit $(KAT_LIMIT)

test-mayo_3: lib/mayo_3.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme MAYO --parameter mayo_3 --library lib/mayo_3.so --manifest kat/mayo_3.tsv --limit $(KAT_LIMIT)

test-mayo_5: lib/mayo_5.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme MAYO --parameter mayo_5 --library lib/mayo_5.so --manifest kat/mayo_5.tsv --limit $(KAT_LIMIT)

../bin/nist-sig:
	$(MAKE) -C ../api

.PHONY: manifest check-results
manifest:
	python3 ../tools/regenerate_manifests.py MAYO

check-results:
	python3 ../tools/run_library_kat.py --scheme MAYO --check-summary --limit $(KAT_LIMIT)
