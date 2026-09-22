# NGCC uniform build: sign-11 FlexTree (reference implementation)
NGCC_ID   := sign-11
NGCC_TYPE := sig
NGCC_ALG  := FlexTree
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Each instance directory is self-contained: params.h includes the matching
# params/params-flextree-*.h, so no -DPARAMS is needed. The default glob
# picks up everything the candidate Makefile lists (address.c auxfunc.c
# counters.c drng.c hash_sm3.c merkle.c pors_fp.c randombytes.c
# SIG_AlgorithmInstance.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
# sign.c) minus the KAT_SIG.c driver. randombytes.c already wraps
# drng_algorithm; drng.c/auxfunc.c are the unmodified ICCS files (CRLF only).
define FT
CFLAGS_$(1) := -std=c99
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call FT,Flextree-160s))
$(eval $(call FT,Flextree-160f))
$(eval $(call FT,Flextree-256s))
$(eval $(call FT,Flextree-256f))
$(eval $(call FT,Flextree-384s))
$(eval $(call FT,Flextree-384f))
$(eval $(call FT,Flextree-512s))
$(eval $(call FT,Flextree-512f))

include ../api/link_finish.mk
