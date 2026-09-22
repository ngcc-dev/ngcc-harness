# NGCC uniform build: sign-18 Origami (reference implementation)
NGCC_ID   := sign-18
NGCC_TYPE := sig
NGCC_ALG  := Origami
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# Candidate Makefile objects: SIG_AlgorithmInstance drng auxfunc symmetric_iccs
# origami_ref aes (+ KAT_SIG driver) with -DORIGAMI_OPT=REF (already the
# header default). The default glob yields exactly that set. Randomness comes
# from drng_algorithm in SIG_AlgorithmInstance.c. The submitted KAT files were
# generated with KAT_NUM_TESTS=1 and contain a single record (Count = 0), so
# the harness is told to produce one record as well.
define ORIGAMI
DEFS_$(1) := -DORIGAMI_OPT=REF
TESTFLAGS_$(1) := --count 1
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call ORIGAMI,Origami-128))
$(eval $(call ORIGAMI,Origami-256))
$(eval $(call ORIGAMI,Origami-384))
$(eval $(call ORIGAMI,Origami-512))

include ../api/link_finish.mk
