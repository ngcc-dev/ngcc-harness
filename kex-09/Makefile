# NGCC uniform build: kex-09 TriQ-KEX (reference implementation)
NGCC_ID   := kex-09
NGCC_TYPE := kex
NGCC_ALG  := TriQ-KEX
include ../api/link_rules.mk

# The submission is nested one level deeper than usual (kex-09/TriQ-KEX/...).
REF := TriQ-KEX/Implementations/Reference_Implementation
NGCC_KAT_DIR := TriQ-KEX/Test_Vectors

# Source list and include paths from each instance's Makefile (SOURCES minus
# KAT_KEX.c). The KEM core lives in src/common (level-independent code) and
# src/ref, with the per-level api.h/parameters.h under src/*/triq-N.
# drng.c/auxfunc.c are the official files; all randomness (including the
# seed of the internal PRNG) is drawn from drng_algorithm in
# KEX_AlgorithmInstance.c. Two-pass protocol.
TRIQ_SRCS := KEX_AlgorithmInstance.c drng.c auxfunc.c \
             src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c src/common/symmetric.c \
             src/ref/gf.c src/ref/gf2x.c src/ref/triq_pke.c src/ref/parsing.c src/ref/reed_muller.c \
             src/ref/reed_solomon.c src/ref/vector.c

# label, level
define TRIQ
SRCS_$(1) := $(TRIQ_SRCS)
INC_$(1)  := -Isrc/$(1)/src/common -Isrc/$(1)/src/common/triq-$(2) -Isrc/$(1)/src/ref -Isrc/$(1)/src/ref/triq-$(2)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call TRIQ,TriQ-KEX-128,128))
$(eval $(call TRIQ,TriQ-KEX-256,256))
$(eval $(call TRIQ,TriQ-KEX-384,384))
$(eval $(call TRIQ,TriQ-KEX-512,512))

include ../api/link_finish.mk

build:
	mkdir -p build

.PHONY: exploit
exploit: | build
	$(CC) -std=c99 -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections \
	  -I '$(REF)/TriQ-KEX-128/src/common' -I '$(REF)/TriQ-KEX-128/src/common/triq-128' \
	  -I '$(REF)/TriQ-KEX-128/src/ref' -I '$(REF)/TriQ-KEX-128/src/ref/triq-128' \
	  -I '$(REF)/TriQ-KEX-128' \
	  -o build/reproduce_ct_sampler reproduce_ct_sampler.c \
	  '$(REF)/TriQ-KEX-128/src/common/symmetric.c' '$(REF)/TriQ-KEX-128/auxfunc.c'
	build/reproduce_ct_sampler
