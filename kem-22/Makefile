# NGCC uniform build: kem-22 Mithril (reference implementation)
NGCC_ID   := kem-22
NGCC_TYPE := kem
NGCC_ALG  := Mithril
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the candidate Makefile (KAT target): arith/{poly,ring,packing,uniform}.c
# pke.c kem.c utils/auxfunc.c utils/drng.c with -DRRLWR_SECURITY_LEVEL=<n>
# -DNDEBUG (the -march=x86-64-v3 tuning flags are dropped; the code is plain C).
# The API header is kem.h (no KEM_AlgorithmInstance.h); the candidate's own
# build names the instance "KEM_RRLWR<n>" while the submitted KAT files are
# KAT_KEM_Mithril-<n>.txt, so the instance name is set to Mithril-<n> here.
# kem.c draws randomness from drng_algorithm directly.
MITHRIL_SRCS := arith/poly.c arith/ring.c arith/packing.c arith/uniform.c pke.c kem.c \
                utils/auxfunc.c utils/drng.c

define MITHRIL
SRCS_$(1) := $(MITHRIL_SRCS)
INC_$(1)  := -Isrc/$(1)/utils -Isrc/$(1)/arith
DEFS_$(1) := -DRRLWR_SECURITY_LEVEL=$(2) -DNDEBUG
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"kem.h"' -DNGCC_INSTANCE='"$(1)"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call MITHRIL,Mithril-128,128))
$(eval $(call MITHRIL,Mithril-256,256))
$(eval $(call MITHRIL,Mithril-512,512))

include ../api/link_finish.mk

# Pinned honest encapsulation / decapsulation mismatch from issue #15.
.PHONY: exploit-decoder
exploit-decoder: build/reproduce_decoding_constant
	build/reproduce_decoding_constant

build/reproduce_decoding_constant: reproduce_decoding_constant.c $(addprefix $(REF)/Mithril-256/,arith/poly.c arith/ring.c arith/packing.c arith/uniform.c pke.c kem.c utils/auxfunc.c utils/drng.c)
	@mkdir -p build
	$(CC) -O2 -DRRLWR_SECURITY_LEVEL=256 -I$(REF)/Mithril-256 -I$(REF)/Mithril-256/utils -I$(REF)/Mithril-256/arith $(filter %.c,$^) -o $@
