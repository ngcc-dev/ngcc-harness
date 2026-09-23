# NGCC uniform build: kem-37 TriQ-KEM (reference implementation)
NGCC_ID   := kem-37
NGCC_TYPE := kem
NGCC_ALG  := TriQ-KEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list and -I paths taken from each instance's Makefile (SOURCES/INCLUDES)
# minus the KAT_KEM.c driver. drng.c/auxfunc.c in the instance dir are the
# official files. KEM_AlgorithmInstance.c seeds the scheme from drng_algorithm.
TRIQ_SRCS := KEM_AlgorithmInstance.c drng.c auxfunc.c \
             src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c src/common/symmetric.c \
             src/ref/gf.c src/ref/gf2x.c src/ref/triq_pke.c src/ref/parsing.c src/ref/reed_muller.c \
             src/ref/reed_solomon.c src/ref/vector.c

# label, level
define TRIQ
SRCS_$(1) := $(TRIQ_SRCS)
INC_$(1)  := -Isrc/$(1)/src/common -Isrc/$(1)/src/common/triq-$(2) -Isrc/$(1)/src/ref -Isrc/$(1)/src/ref/triq-$(2)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call TRIQ,TriQ-KEM-128,128))
$(eval $(call TRIQ,TriQ-KEM-256,256))
$(eval $(call TRIQ,TriQ-KEM-384,384))
$(eval $(call TRIQ,TriQ-KEM-512,512))

include ../api/link_finish.mk

build:
	mkdir -p build

.PHONY: exploit
exploit: | build
	$(CC) -std=c99 -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections \
	  -I '$(REF)/TriQ-KEM-128/src/common' -I '$(REF)/TriQ-KEM-128/src/common/triq-128' \
	  -I '$(REF)/TriQ-KEM-128/src/ref' -I '$(REF)/TriQ-KEM-128/src/ref/triq-128' \
	  -I '$(REF)/TriQ-KEM-128' \
	  -o build/reproduce_ct_sampler reproduce_ct_sampler.c \
	  '$(REF)/TriQ-KEM-128/src/common/symmetric.c' '$(REF)/TriQ-KEM-128/auxfunc.c'
	build/reproduce_ct_sampler
