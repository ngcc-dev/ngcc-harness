# NGCC uniform build: sign-28 SYDO (reference implementation)
NGCC_ID   := sign-28
NGCC_TYPE := sig
NGCC_ALG  := SYDO
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list and defines from each sydo_<level><f|s>/Makefile (COMMON_SOURCES
# of the kat_sig target): -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM
# -DSYDO_REF_NGCC with -I. -Isrc -Ilib -Ilib/blake2/ref (the -O3 -std=c11 of
# that Makefile is replaced by the harness's uniform flags). The six directories
# differ only in SIG_AlgorithmInstance.h (ALGORITHM_INSTANCE "sydo_<...>", which
# selects the row of the parameter table in src/sydo.c) and README.
# Randomness: sig_keygen/sig_sign draw the key-generation seed and the signing
# seed from drng_algorithm and call the *_from_seed functions. src/randomness.c
# (rand_bytes() over a private, fixed-seed DRNG instance) is linked only because
# the unseeded sydo_ref_keygen()/sydo_ref_sign() reference it; the NGCC API never
# calls them.
SYDO_SRCS := SIG_AlgorithmInstance.c \
    $(addprefix src/,aes.c bavc.c compat.c fields.c randomness.c sydo.c field.c hash.c \
        macs.c primitives.c quicksilver.c rsd.c vole_check.c universal_hashing.c \
        utils.c vole.c) \
    lib/auxfunc.c lib/drng.c lib/blake2/ref/blake2b-ref.c

define SYDO
SRCS_$(1) := $(SYDO_SRCS)
INC_$(1)  := -Isrc/$(1)/src -Isrc/$(1)/lib -Isrc/$(1)/lib/blake2/ref
DEFS_$(1) := -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(foreach p,sydo_160f sydo_160s sydo_256f sydo_256s sydo_512f sydo_512s,$(eval $(call SYDO,$(p))))

include ../api/link_finish.mk

.PHONY: exploit-forum-findings exploit-forum-findings-full exploit-static-findings reproduce-fixed-rng
exploit-forum-findings:
	./reproduce_forum_findings.sh

exploit-forum-findings-full:
	./reproduce_forum_findings.sh full

exploit-static-findings:
	python3 reproduce_static_findings.py

SYDO160_DIR := $(REF)/sydo_160s
SYDO160_CORE := src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c \
                src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c \
                src/quicksilver.c src/rsd.c src/vole_check.c src/universal_hashing.c \
                src/utils.c src/vole.c lib/auxfunc.c lib/drng.c lib/blake2/ref/blake2b-ref.c

build/reproduce_fixed_rng: reproduce_fixed_rng.c $(addprefix $(SYDO160_DIR)/,$(SYDO160_CORE))
	@mkdir -p build
	$(CC) -O2 -DNDEBUG -std=c11 -I$(SYDO160_DIR) -I$(SYDO160_DIR)/src -I$(SYDO160_DIR)/lib \
		-I$(SYDO160_DIR)/lib/blake2/ref -DSYDO_REF_NGCC -o $@ $^

reproduce-fixed-rng: build/reproduce_fixed_rng
	@tmp=$$(mktemp -d); trap 'rm -rf "$$tmp"' EXIT; \
		./build/reproduce_fixed_rng > "$$tmp/key1"; \
		./build/reproduce_fixed_rng > "$$tmp/key2"; \
		cmp "$$tmp/key1" "$$tmp/key2"; \
		grep -Eq 'getrandom|/dev/urandom|arc4random_buf|SecRandomCopyBytes|BCryptGenRandom' \
			Implementations/Optimized_Implementation/sydo_160s/src/randomness.c; \
		echo 'ATTACK sign-28-5 CONFIRMED fresh processes generated identical helper-derived keys'
