# Reviewed reference sources; submitted GNUmakefile is not executed.
.DEFAULT_GOAL := all
CC ?= cc
CFLAGS ?= -O2
KAT_LIMIT ?= 0
.PHONY: check-source
check-source:
	test -f 'submission/QR_UOV - Round3/Reference_Implementation/ref/sign.c'
PARAMS := 1q127L3 1q127L10 1q31L3 1q31L10 1q7L10 3q127L3 3q127L10 3q31L3 3q31L10 3q7L10 5q127L3 5q127L10 5q31L3 5q31L10 5q7L10
PRGS := aes shake
INSTANCES := $(foreach p,$(PARAMS),$(foreach x,$(PRGS),$(p)-$(x)))
SOURCES := sign.c qruov_common.c qrop.c rng.c prg.c prim.c linsys.c qruov.c
all: $(addprefix lib/,$(addsuffix .so,$(INSTANCES)))
# Paths containing spaces are shell-quoted; dependency wildcard covers headers.
define instance
lib/$(1)-$(2).so: check-source ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile
	mkdir -p lib
	$$(CC) $$(CFLAGS) -std=c11 -D_DEFAULT_SOURCE -fPIC -shared -Wl,-Bsymbolic -DNIST_SIG_SCHEME='"QR-UOV"' -DNIST_SIG_PARAMETER='"$(1)-$(2)"' -DQRUOV_PARAM_$(1) -DPRG_IS_AES=$(if $(filter aes,$(2)),1,0) -I'submission/QR_UOV - Round3/Reference_Implementation/ref' -I../api $$(addprefix 'submission/QR_UOV - Round3/Reference_Implementation/ref/,$$(addsuffix ',$$(SOURCES))) ../api/nist_sig_shim.c -lcrypto -o $$@
endef
$(foreach p,$(PARAMS),$(foreach x,$(PRGS),$(eval $(call instance,$(p),$(x)))))
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
	python3 ../tools/regenerate_manifests.py QR-UOV
check-results:
	python3 ../tools/run_library_kat.py --scheme QR-UOV --check-summary --limit $(KAT_LIMIT)
$(addprefix test-,$(INSTANCES)): test-%: lib/%.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme QR-UOV --parameter $* --library lib/$*.so --manifest kat/$*.tsv --limit $(KAT_LIMIT)
