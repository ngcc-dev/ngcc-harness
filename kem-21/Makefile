# NGCC uniform build: kem-21 MAMBA-Viper (reference implementation)
NGCC_ID   := kem-21
NGCC_TYPE := kem
NGCC_ALG  := MAMBA-Viper
include ../api/link_rules.mk

# Implementations/Reference_Implementation is identical to the copy under
# API_PKC/API_PKC/Implementations/Reference_Implementation (same KATs).
# Reference_Implementation/AlgorithmInstance/ holds only the ICCS template files
# (not an instance). No shipped binaries are used.
REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per each instance's Makefile: SRC = KEM_AlgorithmInstance.c kem.c viper.c
# viper_arith.c viper_e8.c viper_message_codec.c verify.c drng.c auxfunc.c
# viper_api_rng.c viper_api_hash.c (= default glob minus KAT_KEM.c) with
# -DVIPER_LEVEL=<n> -DVIPER_USE_E8_CODEC=1. viper_api_rng.c maps randombytes()
# onto drng_algorithm; the hashes are built on the ICCS auxfunc pseudoXOF/sm3.
define VIPER
DEFS_$(1) := -DVIPER_LEVEL=$(2) -DVIPER_USE_E8_CODEC=1
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call VIPER,MAMBA-Viper-128,128))
$(eval $(call VIPER,MAMBA-Viper-192,192))
$(eval $(call VIPER,MAMBA-Viper-256,256))
$(eval $(call VIPER,MAMBA-Viper-384,384))
$(eval $(call VIPER,MAMBA-Viper-512,512))

include ../api/link_finish.mk
