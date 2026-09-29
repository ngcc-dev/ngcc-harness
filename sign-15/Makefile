# NGCC uniform build: sign-15 MORNING-ATLAS (reference implementation)
NGCC_ID   := sign-15
NGCC_TYPE := sig
NGCC_ALG  := MORNING-ATLAS
include ../api/link_rules.mk

REF := Implementation/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Candidate Makefile: SIG_lwrdsaNNN.c polyvec.c packing.c poly.c rounding.c
# auxfunc.c drng.c (= default glob minus the KAT_SIG.c driver). The keygen
# draws its seed from drng_algorithm; drng.c/auxfunc.c are the unmodified ICCS
# files. A prebuilt bench_speed ELF is shipped in lwrdsa128/ and is ignored.
#
# Harness result: OVERFLOW on all four instances (candidate-side). sig_sign()
# writes only the detached signature but returns CRYPTO_BYTES + m_len_bytes,
# while sig_get_sn_len_bytes() claims CRYPTO_BYTES; the submitted KATs (Sn_Len
# = 2137, 2145, ... growing with M_Len) were produced by the unmodified ICCS
# driver reading past its sig_get_sn_len_bytes()-sized calloc while printing.
# lwrdsa256/rounding.c and lwrdsa512/poly.c include <immintrin.h> without using
# any intrinsics; on AArch64 compat/aarch64/immintrin.h stands in for it.
MA_COMPAT := $(if $(filter aarch64,$(shell uname -m)),-Icompat/aarch64)

define MA
CFLAGS_$(1) := -std=c99
INC_$(1)    := $(MA_COMPAT)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call MA,lwrdsa128))
$(eval $(call MA,lwrdsa192))
$(eval $(call MA,lwrdsa256))
$(eval $(call MA,lwrdsa512))

include ../api/link_finish.mk

.PHONY: exploit-memory-safety exploit-spec-key-recovery
exploit-memory-safety:
	./reproduce_new_memory_safety.sh

exploit-spec-key-recovery:
	./reproduce_spec_key_recovery.sh
