# NGCC uniform build: sign-17 OPS Digital Signature Algorithm (OPSsig, reference implementation)
NGCC_ID   := sign-17
NGCC_TYPE := sig
NGCC_ALG  := OPS Digital Signature Algorithm
include ../api/link_rules.mk

TOP := Implementations and Test_Vectors
REF := $(TOP)/Implementations/Reference_Implementation

# Source list from each instance Makefile (SOURCES of the kat_sig target, plus
# SIG_AlgorithmInstance.c). The shipped kat_sig binary is ignored. sign.c
# defines its own randombytes() as a wrapper around
# get_random_number(&drng_algorithm, ...); there is no OS entropy source in the
# library sources (drng_algorithm is defined by KAT_SIG.c/test_*.c in the
# submission and by the shim here). The shipped drng.c/auxfunc.c are used.
#
# The reference KATs live in Test_Vectors/reference/OPSsig-<N>/ and are named
# KAT_SIG_OPSsig-<N>-reference.txt while ALGORITHM_INSTANCE is "OPSsig-<N>",
# hence KATDIR_/KATNAME_. The Test_Vectors/avx2/ files belong to the optimized
# implementation and are not tested here.
OPS_SRCS := sign.c mult.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c \
            auxfunc.c drng.c SIG_AlgorithmInstance.c

define OPS
SRCS_$(1) := $(OPS_SRCS)
KATDIR_$(1)  := $(TOP)/Test_Vectors/reference/$(1)
KATNAME_$(1) := $(1)-reference
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call OPS,OPSsig-128))
$(eval $(call OPS,OPSsig-256))
$(eval $(call OPS,OPSsig-512))

include ../api/link_finish.mk

.PHONY: reproduce-spec-findings
reproduce-spec-findings:
	@./reproduce_spec_findings.sh
