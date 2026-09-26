# NGCC uniform build: sign-08 DARTS (reference implementation)
NGCC_ID   := sign-08
NGCC_TYPE := sig
NGCC_ALG  := DARTS
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# HAETAE-derived lattice scheme; one directory per parameter set, selected by
# DARTS_MODE hard-coded in each config.h. Source list = the candidate
# Makefile's SOURCES (SIGN_SRC + symmetric.c + drng.c auxfunc.c +
# SIG_AlgorithmInstance.c). fips202.c ships in the directory but is not in
# their source list (symmetric.c uses the ICCS auxfunc hash), and
# precomputations_rANS/prec_encoding.c is a table generator, so both are left
# out. sign.c defines `DRNG_ctx drng_algorithm;` itself (their build uses
# -fcommon to merge it with KAT_SIG.c's copy), hence -DNGCC_NO_DRNG.
# Test_Vectors also has KAT_SIG_DARTSnnn_AVX2.txt for the optimized tree.
DARTS_SRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c sampler.c encoding.c polyfix.c \
              polymat.c fft.c fixpoint.c symmetric.c drng.c auxfunc.c SIG_AlgorithmInstance.c

define DARTS
SRCS_$(1) := $(DARTS_SRCS)
SHIMDEFS_$(1) := -DNGCC_NO_DRNG
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call DARTS,DARTS128))
$(eval $(call DARTS,DARTS256))
$(eval $(call DARTS,DARTS512))

include ../api/link_finish.mk

.PHONY: exploit-key-recovery
exploit-key-recovery: lib/libDARTS128.so
	./reproduce_key_recovery.sh
