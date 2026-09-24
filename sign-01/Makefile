# NGCC uniform build: sign-01 Aigis-Sig+ (reference implementation)
NGCC_ID   := sign-01
NGCC_TYPE := sig
NGCC_ALG  := Aigis-Sig+
include ../api/link_rules.mk

REF := Implementations/Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from the candidate Makefile's KAT target (SOURCES + DRANDOM):
# -DPARAMS=N -DUSE_ICCS selects the parameter set and the ICCS auxfunc hash.
# rng.c maps randombytes() onto drng_algorithm; randombytes.c (/dev/urandom),
# aigis_sig.c/speed.c/cpucycles.c (benchmark) and ippcpsm3/ + sm3kdf.c
# (Intel IPP SM3 backend, only for USE_SM3) are not built.
# Test_Vectors has KAT_SIG_Aigis-sigN.txt (official "Count =" format) and
# KAT_SIG_Aigis-sigN_iccs.txt (older lowercase format); the harness uses the
# former, which is what ALGORITHM_INSTANCE ("Aigis-sig" STR(PARAMS)) names.
AIGIS_SRCS := sign.c polyvec.c packing.c poly.c reduce.c ntt.c rounding.c fips202.c hashkdf.c \
              auxfunc.c SIG_AlgorithmInstance.c drng.c pspm.c rng.c

# label, dir, PARAMS
define AIGIS
SRCS_$(1) := $(AIGIS_SRCS)
DEFS_$(1) := -DPARAMS=$(3) -DUSE_ICCS
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(2)))
endef

$(eval $(call AIGIS,Aigis-sig1,Aigis-Sig+-I,1))
$(eval $(call AIGIS,Aigis-sig2,Aigis-Sig+-II,2))
$(eval $(call AIGIS,Aigis-sig3,Aigis-Sig+-III,3))

include ../api/link_finish.mk

.PHONY: exploit-memory
exploit-memory:
	bash reproduce_memory_safety.sh
