# NGCC uniform performance builds: BiT, submitted reference and AVX2 variants.
NGCC_ID   := sign-02
NGCC_TYPE := sig
NGCC_ALG  := BiT
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
OPT := Implementations/Optimized_Implementation

BIT_REF_SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c fips202.c ntt.c \
                packing.c poly.c polyvec.c sample.c sign.c symmetric.c
BIT_AVX_SRCS := SIG_AlgorithmInstance.c auxfunc.c consts.c drng.c fips202.c \
                fips202x4.c packing.c poly.c polyvec.c sample.c \
                sign.c symmetric.c ntt.S intt.S pointwise.S f1600x4.S
BIT_AVX_SRCS_128 := $(BIT_AVX_SRCS) shim_asm.c
BIT_AVX_SRCS_256 := $(BIT_AVX_SRCS) shim_asm.c
BIT_AVX_SRCS_512 := $(BIT_AVX_SRCS)
BIT_AVX_LDLIBS_128 := -Wl,--defsym=ntt_avx_forward=ntt_fwd_asm -Wl,--defsym=ntt_avx_inverse=ntt_inv_asm
BIT_AVX_LDLIBS_256 := $(BIT_AVX_LDLIBS_128)

define BIT_REF
SRCS_BiT-$(1) := $(BIT_REF_SRCS)
$$(eval $$(call ngcc_instance,BiT-$(1),$(REF)/BiT-$(1)))
endef
$(eval $(call BIT_REF,128))
$(eval $(call BIT_REF,256))
$(eval $(call BIT_REF,512))

ifeq ($(PERF_OPT),1)
define BIT_AVX
SRCS_BiT-$(1)-avx2 := $(BIT_AVX_SRCS_$(1))
# The submitted 128/256 assembler exports ntt_fwd_asm/ntt_inv_asm while its C
# calls ntt_avx_forward/ntt_avx_inverse. The 512 assembler exports the expected
# names directly. Link-time aliases adapt 128/256 without editing source.
LDLIBS_BiT-$(1)-avx2 := $(BIT_AVX_LDLIBS_$(1))
KATNAME_BiT-$(1)-avx2 := BiT-$(1)
MANIFESTLABEL_BiT-$(1)-avx2 := BiT-$(1)
$$(eval $$(call ngcc_instance,BiT-$(1)-avx2,$(OPT)/BiT-$(1)-avx2))
endef
$(eval $(call BIT_AVX,128))
$(eval $(call BIT_AVX,256))
$(eval $(call BIT_AVX,512))
endif

include ../api/link_finish.mk
