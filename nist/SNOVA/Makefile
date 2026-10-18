# Reviewed reference sources; submitted build scripts are not executed.
.DEFAULT_GOAL := all
CC ?= cc
CFLAGS ?= -O2
KAT_LIMIT ?= 0
REF := submission/SNOVA_Round3/Reference_Implementation
INSTANCES := SNOVA_I_K SNOVA_I_B SNOVA_I_S SNOVA_III_K SNOVA_III_B SNOVA_III_S SNOVA_V_K SNOVA_V_B SNOVA_V_S
INSTANCES += $(addsuffix _AES,$(INSTANCES))
SOURCES := snova_ref.c symmetric_ref.c rng.c sign.c aes.c
all: $(addprefix lib/,$(addsuffix .so,$(INSTANCES)))
define instance
lib/$(1).so: $$(addprefix $$(REF)/$(1)/,$$(SOURCES)) $$(wildcard $$(REF)/$(1)/*.h) ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile
	mkdir -p lib
	$$(CC) $$(CFLAGS) -std=c11 -fPIC -shared -Wl,-Bsymbolic -DSNOVA_REF=1 -DNIST_SIG_SCHEME='"SNOVA"' -DNIST_SIG_PARAMETER='"$(1)"' -I$$(REF)/$(1) -I../api $$(addprefix $$(REF)/$(1)/,$$(SOURCES)) ../api/nist_sig_shim.c -o $$@
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
	python3 ../tools/regenerate_manifests.py SNOVA
check-results:
	python3 ../tools/run_library_kat.py --scheme SNOVA --check-summary --limit $(KAT_LIMIT)
$(addprefix test-,$(INSTANCES)): test-%: lib/%.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme SNOVA --parameter $* --library lib/$*.so --manifest kat/$*.tsv --limit $(KAT_LIMIT)
