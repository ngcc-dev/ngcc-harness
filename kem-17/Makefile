# NGCC uniform build: kem-17 HEP-QC (reference implementation)
NGCC_ID   := kem-17
NGCC_TYPE := kem
NGCC_ALG  := HEP-QC
include ../api/link_rules.mk

IMPL := Implementations/Implementations
REF  := $(IMPL)/ref

# Layout per Implementations/CMakeLists.txt (HEP_QC_ARCH=ref): the algorithm is
# Implementations/common/*.c plus Implementations/ref/{KEM,PKE}_HEP_QC.c, and the
# per-instance directory ref/hep-qc-<n> holds only parameters.h/reed_solomon.h
# (common/hep-qc-<n>/api.h has the sizes). The instance dir is used as source
# dir; sources are referenced relative to it. Defines: -DHEP_QC_ARCH_REF=1.
# The API header is KEM_HEP_QC.h and its ALGORITHM_INSTANCE is the placeholder
# "AlgorithmInstance" for all four instances, so the submitted KAT files are
# Test_Vectors/hep-qc-<n>/KAT_KEM_AlgorithmInstance.txt (KATDIR_/KATNAME_) and
# the metadata instance name is set to hep-qc-<n> with -DNGCC_INSTANCE.
# Lib/drng and Lib/auxfunc are byte-identical to the official copies modulo
# CRLF; the official api/ copies are linked. The x86_64 tree is not built.
# NOTE (candidate side): the scheme never calls get_random_number(); keygen/
# encaps take randomness from a global SHAKE-based PRNG context in
# common/symmetric.c that the KAT flow never seeds (prng_init is only called
# by the intermediate-values test), so the per-count Seed is unused. The
# shipped KAT_KEM.c driver is also reduced to 10 counts (matches --count 10).
HEP_SRCS := ../KEM_HEP_QC.c ../PKE_HEP_QC.c \
  ../../common/code.c ../../common/crypto_memset.c ../../common/fft.c ../../common/gf.c \
  ../../common/gf2x.c ../../common/parsing.c ../../common/reed_muller.c \
  ../../common/reed_solomon.c ../../common/symmetric.c ../../common/vector.c

# common/parsing.h and common/vector.h include <immintrin.h> without using any
# intrinsics; on AArch64 compat/aarch64/immintrin.h stands in for it.
HEP_COMPAT := $(if $(filter aarch64,$(shell uname -m)),-Icompat/aarch64)

define HEP
SRCS_hep-qc-$(1) := $(HEP_SRCS)
INC_hep-qc-$(1)  := -I$(IMPL)/common -I$(IMPL)/common/hep-qc-$(1) -I$(REF) $(HEP_COMPAT)
DEFS_hep-qc-$(1) := -DHEP_QC_ARCH_REF=1
CFLAGS_hep-qc-$(1) := -std=c99
SHIMDEFS_hep-qc-$(1) := -DNGCC_INSTANCE_HEADER='"KEM_HEP_QC.h"' -DNGCC_INSTANCE='"hep-qc-$(1)"'
KATDIR_hep-qc-$(1) := Test_Vectors/hep-qc-$(1)
KATNAME_hep-qc-$(1) := AlgorithmInstance
$$(eval $$(call ngcc_instance,hep-qc-$(1),$(REF)/hep-qc-$(1)))
endef

$(eval $(call HEP,1))
$(eval $(call HEP,3))
$(eval $(call HEP,5))
$(eval $(call HEP,7))

include ../api/link_finish.mk
