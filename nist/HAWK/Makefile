# Harness-owned build: compile submitted source; do not execute upstream Makefiles.
CC ?= cc
CFLAGS ?= -O2 -g -std=c99 -Wall -Wextra
CPPFLAGS ?=
LDFLAGS ?=
KAT_LIMIT ?= 0
REF := submission/NIST-SUBMISSION/Reference_Implementation
PARAMETERS := hawk256 hawk512 hawk1024
SOURCES := api.c hawk_kgen.c hawk_sign.c hawk_vrfy.c ng_fxp.c ng_hawk.c ng_mp31.c ng_ntru.c ng_poly.c ng_zint31.c sha3.c rng.c
LIBRARIES := $(addprefix lib/lib,$(addsuffix .so,$(PARAMETERS)))
.PHONY: all libs test manifest clean
all: libs
libs: $(LIBRARIES)
lib/lib%.so: $(addprefix $(REF)/%/,$(SOURCES)) $(wildcard $(REF)/*/*.h) ../api/nist_sig_shim.c ../api/nist_sig_api.h
	mkdir -p lib
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -Wl,-Bsymbolic -Wl,-z,defs -I../api -I$(REF)/$* \
	  -DNIST_SIG_SCHEME='"HAWK"' -DNIST_SIG_PARAMETER='"$*"' \
	  $(addprefix $(REF)/$*/,$(SOURCES)) ../api/nist_sig_shim.c $(LDFLAGS) -o $@
../bin/nist-sig:
	$(MAKE) -C ../api harness
test: libs ../bin/nist-sig
	@mkdir -p results; : > results/summary.tsv; rc=0; \
	for p in $(PARAMETERS); do \
	  ../bin/nist-sig kat lib/lib$$p.so kat/$$p.tsv $(if $(filter 0,$(KAT_LIMIT)),,$(KAT_LIMIT)) > results/$$p.log 2>&1; status=$$?; \
	  tail -1 results/$$p.log | tee -a results/summary.tsv; \
	  if [ $$status -ne 0 ]; then rc=1; fi; \
	done; exit $$rc
manifest:
	python3 ../tools/kat_manifest.py --input ../.cache/extracted/HAWK/NIST-SUBMISSION/KAT/PQCsignKAT_96.rsp --output kat/hawk256.tsv
	python3 ../tools/kat_manifest.py --input ../.cache/extracted/HAWK/NIST-SUBMISSION/KAT/PQCsignKAT_184.rsp --output kat/hawk512.tsv
	python3 ../tools/kat_manifest.py --input ../.cache/extracted/HAWK/NIST-SUBMISSION/KAT/PQCsignKAT_360.rsp --output kat/hawk1024.tsv
clean:
	rm -rf lib results
