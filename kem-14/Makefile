# NGCC uniform build: kem-14 DTRU (reference implementation)
NGCC_ID   := kem-14
NGCC_TYPE := kem
NGCC_ALG  := DTRU
include ../api/link_rules.mk

IMPL := Implementations and Test_Vectors
REF  := $(IMPL)/Implementations/Reference_Implementation
NGCC_KAT_DIR := $(IMPL)/Test_Vectors

# Each instance directory is self-contained; the default glob equals the
# SOURCES list of the candidate Makefile (test_*.c and KAT_KEM.c are dropped).
# KEM_AlgorithmInstance.c draws from the shared drng_algorithm; drng.c and
# auxfunc.c are the unmodified API_PKC copies. The Optimized (AVX2) trees and
# the Additional_Implementation ARM/mupq trees are not built.
$(eval $(call ngcc_instance,DTRU-648,$(REF)/DTRU-648))
$(eval $(call ngcc_instance,DTRU-768,$(REF)/DTRU-768))
$(eval $(call ngcc_instance,DTRU-1024,$(REF)/DTRU-1024))
$(eval $(call ngcc_instance,DTRU-1536,$(REF)/DTRU-1536))
$(eval $(call ngcc_instance,DTRU-2048,$(REF)/DTRU-2048))
$(eval $(call ngcc_instance,DTRU-Light,$(REF)/DTRU-Light))
$(eval $(call ngcc_instance,DTRU-Prime,$(REF)/DTRU-Prime))

# Second KAT set: the same five instances built with -DPK_PACK_OPT=1 (extreme
# public-key compression, Makefile target KAT_KEM_PACK_PK), compared against
# Test_Vectors_PACK_PK/KAT_KEM_DTRU-<n>.txt. ALGORITHM_INSTANCE is unchanged in
# the source; the metadata instance name gets a -PACK_PK suffix so the RESULT
# lines are unambiguous, and KATNAME_ maps back to the submitted file name.
define DTRU_PACK
DEFS_DTRU-$(1)-PACK_PK := -DPK_PACK_OPT=1
KATDIR_DTRU-$(1)-PACK_PK := $(IMPL)/Test_Vectors_PACK_PK
KATNAME_DTRU-$(1)-PACK_PK := DTRU-$(1)
SHIMDEFS_DTRU-$(1)-PACK_PK := -DNGCC_INSTANCE='"DTRU-$(1)-PACK_PK"'
$$(eval $$(call ngcc_instance,DTRU-$(1)-PACK_PK,$(REF)/DTRU-$(1)))
endef
$(eval $(call DTRU_PACK,648))
$(eval $(call DTRU_PACK,768))
$(eval $(call DTRU_PACK,1024))
$(eval $(call DTRU_PACK,1536))
$(eval $(call DTRU_PACK,2048))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libDTRU-Light.so lib/libDTRU-1024.so
	python3 reproduce_oversize_decapsulation.py
