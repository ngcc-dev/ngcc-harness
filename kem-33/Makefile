# NGCC uniform build: kem-33 QUBE (reference implementation)
NGCC_ID   := kem-33
NGCC_TYPE := kem
NGCC_ALG  := QUBE
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from each instance's CMakeLists.txt (COMMON_SRCS + REF_SRCS +
# API_PKC_SRCS; lib/api_pkc holds the official drng.c/auxfunc.c, auxfunc.c
# differing only by line endings). test/ holds the drivers. src/common/kem.c
# draws from drng_algorithm.
# The API header is src/common/kem_qube.h (not KEM_AlgorithmInstance.h) and
# defines ALGORITHM_INSTANCE "qube"; the candidate's KAT_KEM.c appends
# "_<PARAM_SECURITY>" to the file name, so the shim is given the full
# instance name to match KAT_KEM_qube_<N>.txt in Test_Vectors.
# RESULT NOTE (candidate side): this reference tree reproduces its own in-tree
# qube-*/KAT/KAT_KEM_qube_<N>.txt files byte for byte (verified), but the
# official Test_Vectors/ files differ: they carry the key/ciphertext sizes of
# the SHAKE-256-based Optimized_Implementation (qube-1/3/4/5, e.g. PK 3310 vs
# 3294 for 128) and there is no KAT for qube-192 at all. The submitted KATs
# were evidently generated from the optimized code, not from this
# "PDF-aligned" SM3 reference; hence MISMATCH x4 and NOKAT for qube-192.
QUBE_SRCS := src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c \
             src/common/kem_qube.c src/common/symmetric.c \
             src/ref/gf.c src/ref/gf2x.c src/ref/parsing.c src/ref/qube.c src/ref/reed_muller.c \
             src/ref/reed_solomon.c src/ref/vector.c \
             lib/api_pkc/auxfunc.c lib/api_pkc/drng.c

# label, security level
define QUBE
SRCS_$(1) := $(QUBE_SRCS)
INC_$(1)  := -Isrc/$(1)/src/common -Isrc/$(1)/src/ref -Isrc/$(1)/lib/api_pkc
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"kem_qube.h"' -DNGCC_INSTANCE='"qube_$(2)"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call QUBE,qube-128,128))
$(eval $(call QUBE,qube-192,192))
$(eval $(call QUBE,qube-256,256))
$(eval $(call QUBE,qube-384,384))
$(eval $(call QUBE,qube-512,512))

include ../api/link_finish.mk

build:
	mkdir -p build

.PHONY: exploit
exploit: | build
	$(CC) -std=c99 -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections \
	  -I '$(REF)/qube-256/src/common' -I '$(REF)/qube-256/src/common/qube-256' \
	  -I '$(REF)/qube-256/src/ref' -I '$(REF)/qube-256/src/ref/qube-256' \
	  -I '$(REF)/qube-256/lib/api_pkc' -I '$(REF)/qube-256' \
	  -o build/reproduce_ct_sampler reproduce_ct_sampler.c \
	  '$(REF)/qube-256/src/common/symmetric.c' '$(REF)/qube-256/src/common/crypto_memset.c' \
	  '$(REF)/qube-256/lib/api_pkc/auxfunc.c'
	build/reproduce_ct_sampler
