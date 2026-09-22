# NGCC uniform build: sign-34 YuanYang.DSA (reference implementation)
NGCC_ID   := sign-34
NGCC_TYPE := sig
NGCC_ALG  := YuanYang.DSA
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from each instance Makefile (COMMON_SRC + SIG_SRC + SIG_NTRUGEN_SRC,
# i.e. exactly what its KAT_SIG target links). drng_algorithm.c (which only
# defines `DRNG_ctx drng_algorithm;`) is NOT linked: the shim provides it, as in
# the candidate's own KAT_SIG build. The shipped drng.c/auxfunc.c are identical
# to api/ (modulo CRLF). prng.c draws from drng_algorithm by default; -DNGCC_KATS
# mirrors the candidate's kat_ngcc target and hard-disables the optional
# YUANYANG_FORCE_SYSTEM_RANDOMNESS getrandom() path. The ntrugen twiddle table
# file differs per degree (twiddles9/10/11.c). KAT file names use "YuanYang-N"
# while ALGORITHM_INSTANCE is "yuanyang-N", hence KATNAME_.
YY_SRCS := common.c prng.c fpr.c fft.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c \
           SIG_AlgorithmInstance.c sign.c sign_ntt.c vrfy.c sampler.c \
           keygen/keygen.c keygen/pairgen.c keygen/perturbation.c keygen/fft.c keygen/security_loss.c \
           keygen/ntrugen/src/ntrugen.c keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c \
           keygen/ntrugen/src/cfft.c keygen/ntrugen/src/complex.c keygen/ntrugen/src/fft.c

# label, twiddle file
define YY
SRCS_$(1) := $(YY_SRCS) keygen/ntrugen/src/$(2)
INC_$(1)  := -Isrc/$(1)/keygen/ntrugen/src
DEFS_$(1) := -DNGCC_KATS
CFLAGS_$(1) := -std=c99
KATNAME_$(1) := YuanYang-$(3)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call YY,yuanyang-512,twiddles9.c,512))
$(eval $(call YY,yuanyang-1024,twiddles10.c,1024))
$(eval $(call YY,yuanyang-2048,twiddles11.c,2048))

# Public-data witnesses submitted in ngcc-harness PR #3.  Build the source;
# the prebuilt executable from the PR is intentionally not imported.
reproduce_transcript_leak: reproduce_transcript_leak.c $(API)/link_common.h $(API)/link_sig.h
	$(CC) -O2 -std=gnu11 -Wall -Wextra -I$(API) -o $@ $< -ldl -lm

.PHONY: exploit clean-exploit
exploit: lib/libyuanyang-512.so reproduce_transcript_leak

clean-exploit:
	rm -f reproduce_transcript_leak
clean: clean-exploit

include ../api/link_finish.mk
