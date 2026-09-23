# NGCC uniform build: kem-27 NTRE (reference implementation)
NGCC_ID   := kem-27
NGCC_TYPE := kem
NGCC_ALG  := NTRE
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the candidate Makefile: src/poly.c src/ntt.c src/symmetric.c
# KEM_AlgorithmInstance.c auxfunc.c drng.c (own copies identical to api/).
# KEM_AlgorithmInstance.c draws from drng_algorithm. bin/ (prebuilt KAT_KEM)
# and test/ (test driver + prebuilt test_ntre) are not used.
NTRE_SRCS := src/poly.c src/ntt.c src/symmetric.c KEM_AlgorithmInstance.c auxfunc.c drng.c

define NTRE
SRCS_$(1) := $(NTRE_SRCS)
INC_$(1)  := -Isrc/$(1)/src
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call NTRE,NTRE-128))
$(eval $(call NTRE,NTRE-256))
$(eval $(call NTRE,NTRE-512))

include ../api/link_finish.mk

.PHONY: exploit
exploit:
	python3 reproduce_seed_ceiling.py
