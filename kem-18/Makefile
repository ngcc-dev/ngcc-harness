# NGCC uniform build: kem-18 LoongKEM (reference implementation)
NGCC_ID   := kem-18
NGCC_TYPE := kem
NGCC_ALG  := LoongKEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the candidate makefile: auxfunc.c drng.c poly.c KEM_Loong.c (default glob,
# KAT_KEM.c excluded). The API header is named KEM_Loong.h instead of
# KEM_AlgorithmInstance.h. The shipped drng.c/auxfunc.c are identical to api/.
# Randomness: KEM_Loong.c calls get_random_number(&drng_algorithm, ...).
define LOONG
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"KEM_Loong.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call LOONG,Loong128))
$(eval $(call LOONG,Loong256))
$(eval $(call LOONG,Loong384))
$(eval $(call LOONG,Loong512))

include ../api/link_finish.mk

.PHONY: reproduce-public-recovery
reproduce-public-recovery:
	sh reproduce_public_recovery.sh
