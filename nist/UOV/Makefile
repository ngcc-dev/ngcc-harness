# Reviewed reference sources; no submitted Makefile is executed.
.DEFAULT_GOAL := all
CC ?= cc
CFLAGS ?= -O2
KAT_LIMIT ?= 0
REF := submission/UOV-R3/Reference_Implementation
INSTANCES := Ip Is III V Ip_pkc Is_pkc III_pkc V_pkc Ip_pkc_skc Is_pkc_skc III_pkc_skc V_pkc_skc
SOURCES := parallel_matrix_op.c blas_matrix_ref.c fips202.c blas_matrix.c sign.c ov_keypair.c utils_prng.c ov_keypair_computation.c ov_publicmap.c utils_hash.c utils_randombytes.c aes128_4r_ffs.c utils.c ov.c
all: $(addprefix lib/,$(addsuffix .so,$(INSTANCES)))
define instance
lib/$(1).so: $$(addprefix $$(REF)/$(1)/,$$(SOURCES)) $$(wildcard $$(REF)/$(1)/*.h) $$(REF)/nistkat/rng.c ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile
	mkdir -p lib
	$$(CC) $$(CFLAGS) -std=c99 -fPIC -shared -Wl,-Bsymbolic -D_NIST_KAT_ -DNIST_SIG_SCHEME='"UOV"' -DNIST_SIG_PARAMETER='"$(1)"' -I$$(REF)/$(1) -I$$(REF)/nistkat -I../api $$(addprefix $$(REF)/$(1)/,$$(SOURCES)) $$(REF)/nistkat/rng.c ../api/nist_sig_shim.c -lcrypto -o $$@
endef
$(foreach p,$(INSTANCES),$(eval $(call instance,$(p))))
.PHONY: all clean
clean:
	rm -rf lib results

.PHONY: libs test
libs: all
test: libs ../bin/nist-sig
	KAT_LIMIT=$(KAT_LIMIT) python3 run_tests.py

../bin/nist-sig:
	$(MAKE) -C ../api harness

.PHONY: manifest check-results $(addprefix test-,$(INSTANCES))
manifest:
	python3 ../tools/regenerate_manifests.py UOV
check-results:
	python3 ../tools/run_library_kat.py --scheme UOV --check-summary --limit $(KAT_LIMIT)
$(addprefix test-,$(INSTANCES)): test-%: lib/%.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme UOV --parameter $* --library lib/$*.so --manifest kat/$*.tsv --limit $(KAT_LIMIT)
