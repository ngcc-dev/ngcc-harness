# Staged Scloud+ SHAKE-128 performance integration. Run from kem-35:
#   make -C kem-35 -f ../performance/kem-35.mk
# Source selection mirrors the submitted leaf CMake files, but executes none of
# their build scripts. The AVX2 and NEON variants share the same API and data.
NGCC_ID   := kem-35
NGCC_TYPE := kem
NGCC_ALG  := Scloud+
include ../api/link_rules.mk

BASE := Implementations and Test_Vectors/Implementations
REF  := $(BASE)/Reference_Implementation
OPT  := $(BASE)/Optimized_Implementation
SHARED_REL := ../../../_shared
SC_COMMON := $(addprefix $(SHARED_REL)/scloudplus_core/common/,encode.c hash_aes_shake.c kem.c pke.c sample.c util.c)
SC_API := $(SHARED_REL)/api_pkc/kat_random.c $(SHARED_REL)/api_pkc/drng.c
NGCC_KAT_DIR := ../orig/kem-35/Implementations and Test_Vectors/Test_Vectors

define SCLOUD
SRCS_$(1) := KEM_AlgorithmInstance.c $(SC_COMMON) $(SC_API) $(2)
INC_$(1) := -Isrc/$(1)/$(SHARED_REL)/api_pkc \
             -Isrc/$(1)/$(SHARED_REL)/scloudplus_core/include \
             -Isrc/$(1)/$(SHARED_REL)/scloudplus_core/common
DEFS_$(1) := -DSCLOUDPLUS_FAMILY_SHAKE $(3)
NOAUX_$(1) := 1
KATNAME_$(1) := Scloudplus-128-SHAKE-packed10
MANIFESTLABEL_$(1) := Scloudplus-128-SHAKE-packed10
$$(eval $$(call ngcc_instance,$(1),$(4)))
endef

SC_REF := $(SHARED_REL)/scloudplus_core/ref/matrix_reference.c \
          $(SHARED_REL)/scloudplus_core/ref/pack_reference.c
$(eval $(call SCLOUD,Scloudplus-128-SHAKE-ref,$(SC_REF),-DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_SHAKE,$(REF)/Scloudplus-128/kem))

ifeq ($(PERF_AVX2),1)
SC_AVX := $(SHARED_REL)/scloudplus_core/avx2/matrix_avx2.c \
          $(SHARED_REL)/scloudplus_core/ref/pack_reference.c \
          $(SHARED_REL)/scloudplus_core/avx2/sample_avx2.c \
          $(SHARED_REL)/scloudplus_core/avx2/fips202x4.c \
          $(SHARED_REL)/scloudplus_core/avx2/keccak4x/KeccakP-1600-times4-SIMD256.c
$(eval $(call SCLOUD,Scloudplus-128-SHAKE-avx2,$(SC_AVX),-DSCLOUDPLUS_TIER_OPTIMIZED -DSCLOUDPLUS_BACKEND_AVX2 -DSCLOUDPLUS_AVX2_FAMILY_SHAKE,$(OPT)/Scloudplus-128/kem))
endif

ifeq ($(PERF_NEON),1)
SC_NEON := $(SHARED_REL)/scloudplus_core/neon/matrix_neon.c \
           $(SHARED_REL)/scloudplus_core/neon/pack_neon.c \
           $(SHARED_REL)/scloudplus_core/neon/sample_neon.c
$(eval $(call SCLOUD,Scloudplus-128-SHAKE-neon,$(SC_NEON),-DSCLOUDPLUS_TIER_OPTIMIZED -DSCLOUDPLUS_BACKEND_NEON -DSCLOUDPLUS_NEON_FAMILY_SHAKE,$(OPT)/Scloudplus-128/kem))
endif

include ../api/link_finish.mk
