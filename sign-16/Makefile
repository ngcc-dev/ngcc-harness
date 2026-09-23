# NGCC uniform build: sign-16 Octarine (reference implementation)
NGCC_ID   := sign-16
NGCC_TYPE := sig
NGCC_ALG  := Octarine
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list taken from each instance's Makefile (SOURCES_RRLWR + SOURCES_UTILS).
# utils/drng.c and utils/auxfunc.c are byte-identical to api/.  The ICCS API
# lives in sign.c/sign.h (no SIG_AlgorithmInstance.[ch]); sign.h defines
# ALGORITHM_INSTANCE as "SIGN_RRLWR" for every level, while the submitted KAT
# files are named KAT_SIG_Octarine-<level>.txt, so the instance name is set
# explicitly with -DNGCC_INSTANCE.  Randomness is drawn from drng_algorithm
# (GENERATE_RANDOM_BYTES in sign.c); no OS entropy is used.
#
# Known result: MISMATCH on every level.  The submitted Test_Vectors were
# generated with OUTPUT_BLANK_TEST_VECTORS=1 (the ICCS template default, still
# set in sign.h), so their PK_Len/SK_Len/Sn_Len lines are blank.  All Seed, PK,
# SK, M and Sn hex values match our output byte for byte and the claimed sizes
# match the hex lengths; the harness is not relaxed to hide this.
OCT_SRCS := arith/fprime.c arith/poly.c arith/ring.c arith/packing.c arith/uniform.c \
            sign_ring.c sign.c crt.c utils/auxfunc.c utils/drng.c

# label, level
define OCT
SRCS_$(1) := $(OCT_SRCS)
INC_$(1)  := -Isrc/$(1)/arith -Isrc/$(1)/utils
DEFS_$(1) := -DRRLWR_SECURITY_LEVEL=$(2)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"sign.h"' -DNGCC_INSTANCE='"$(1)"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call OCT,Octarine-128,128))
$(eval $(call OCT,Octarine-256,256))
$(eval $(call OCT,Octarine-512,512))

include ../api/link_finish.mk
