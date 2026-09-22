# NGCC uniform build: kem-40 YuanYang.KEM (reference implementation)
NGCC_ID   := kem-40
NGCC_TYPE := kem
NGCC_ALG  := YuanYang.KEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation

# Source list per the candidate Makefile (KEM_CORE_SRC = prng.c auxfunc.c
# drng.c ntt.c codec.c poly_inv_ntt.c KEM_AlgorithmInstance.c kem.c
# kem-keygen.c); drng.c/auxfunc.c are the official files. drng_algorithm.c
# (defines the global DRNG_ctx) is left out because the shim defines it; the
# candidate's own KAT build does not link it either (KAT_KEM.c defines it).
# The candidate flags -fopenmp/-Ofast are not used.
#
# Layout anomalies: (1) the submitted KAT files are named yuanyang-<d>.txt
# without the KAT_KEM_ prefix, so kat_ref/ holds symlinks with the expected
# names (created here, outside the submission tree); (2) all three instances
# define ALGORITHM_INSTANCE "YuanYang-KEM", so the libraries are labelled by
# directory name and KATNAME_ selects the right file.
YY_SRCS := prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c KEM_AlgorithmInstance.c kem.c kem-keygen.c

$(shell mkdir -p kat_ref; for d in 512 1024 2048; do ln -sfn "$(CURDIR)/Test_Vectors/yuanyang-$$d.txt" "kat_ref/KAT_KEM_yuanyang-$$d.txt"; done)

define YY
SRCS_$(1) := $(YY_SRCS)
KATDIR_$(1) := kat_ref
KATNAME_$(1) := $(1)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call YY,yuanyang-512))
$(eval $(call YY,yuanyang-1024))
$(eval $(call YY,yuanyang-2048))

include ../api/link_finish.mk
