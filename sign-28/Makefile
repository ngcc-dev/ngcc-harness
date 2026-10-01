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

.PHONY: exploit-forum-findings exploit-forum-findings-full exploit-static-findings
exploit-forum-findings:
	./reproduce_forum_findings.sh

exploit-forum-findings-full:
	./reproduce_forum_findings.sh full

exploit-static-findings:
	python3 reproduce_static_findings.py
