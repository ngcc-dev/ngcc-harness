# NGCC uniform build: kem-08 BW-KEM (reference implementation)
NGCC_ID   := kem-08
NGCC_TYPE := kem
NGCC_ALG  := BW-KEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors/Reference_Implementation

# Source list = CORE_SRC + API_COMMON_SRC from each instance's Makefile (the
# default glob gives exactly that once KAT_KEM.c/test_speed.c are dropped);
# per-instance -D flags from its CPPFLAGS. drng.c/auxfunc.c are byte-identical
# to api/. KEM_AlgorithmInstance.c draws coins from drng_algorithm and calls
# the *_derand core functions; randombytes_api.c (getrandom) is reached only
# from the unused non-derand crypto_kem_* wrappers and is compiled to satisfy
# the symbol. drng_algorithm is defined only in the excluded test_speed.c /
# KAT_KEM.c, so the shim provides it. KAT files are in
# Test_Vectors/Reference_Implementation/.

DEFS_BW_KEM_C128 := -DBWKEM128_INTERNAL_COMPAT -DBWKEM128_USE_ICCS_AUXFUNC
CFLAGS_BW_KEM_C128 := -std=c99
$(eval $(call ngcc_instance,BW_KEM_C128,$(REF)/BW_KEM_C128))

DEFS_BW_KEM_C256 := -DBWKEM_C256_USE_ICCS_AUXFUNC
CFLAGS_BW_KEM_C256 := -std=c99
$(eval $(call ngcc_instance,BW_KEM_C256,$(REF)/BW_KEM_C256))

# Anomaly: BW_KEM_C512/KAT_KEM.c was modified to loop 100 times instead of the
# official 10, and the submitted KAT_KEM_BW_KEM_C512.txt has 100 records
# (records 0-9 match the official schedule); --count 100 checks all of them.
DEFS_BW_KEM_C512 := -DBWKEM_C512_USE_ICCS_AUXFUNC
CFLAGS_BW_KEM_C512 := -std=c99
TESTFLAGS_BW_KEM_C512 := --count 100
$(eval $(call ngcc_instance,BW_KEM_C512,$(REF)/BW_KEM_C512))

include ../api/link_finish.mk
