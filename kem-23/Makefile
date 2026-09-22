# NGCC uniform build: kem-23 Mito (reference implementation)
NGCC_ID   := kem-23
NGCC_TYPE := kem
NGCC_ALG  := Mito
include ../api/link_rules.mk

# Implementations/ and Test_Vector/ are duplicated byte-for-byte under 中文资料/.
REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vector

# No candidate Makefile; README says "gcc *.c -O2", so the default glob (minus
# KAT_KEM.c) is used. ALGORITHM_INSTANCE comes from parameters.h as
# "MITO_<x>_<y>" (KAT files follow that spelling); labels use the directory
# names. symmetric.c wraps get_random_number(&drng_algorithm, ...) as
# prng_get_bytes(); its prng_init() (which would reseed drng_algorithm) is
# never called by the KEM code.
$(eval $(call ngcc_instance,Mito-1-128,$(REF)/Mito-1-128))
$(eval $(call ngcc_instance,Mito-1-256,$(REF)/Mito-1-256))
$(eval $(call ngcc_instance,Mito-1-512,$(REF)/Mito-1-512))
$(eval $(call ngcc_instance,Mito-1-E-128,$(REF)/Mito-1-E-128))
$(eval $(call ngcc_instance,Mito-1-E-256,$(REF)/Mito-1-E-256))
$(eval $(call ngcc_instance,Mito-1-E-512,$(REF)/Mito-1-E-512))
$(eval $(call ngcc_instance,Mito-2-E-128,$(REF)/Mito-2-E-128))
$(eval $(call ngcc_instance,Mito-2-E-256,$(REF)/Mito-2-E-256))
$(eval $(call ngcc_instance,Mito-2-E-512,$(REF)/Mito-2-E-512))

include ../api/link_finish.mk
