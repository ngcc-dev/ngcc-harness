# NGCC uniform build: sign-12 Galas Signature Scheme (reference implementation)
NGCC_ID   := sign-12
NGCC_TYPE := sig
NGCC_ALG  := Galas
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# The eight instance directories are byte-identical (only INSTANCE.txt
# differs); the parameter set is selected at compile time with
# -DGALAS_INSTANCE=GALAS_<n><S|F> -DGALAS_INSTANCE_NAME="Galas-<n><S|F>"
# (ngcc/SIG_AlgorithmInstance.h defaults to GALAS_256S otherwise). Source list
# = SIG_SRCS from the candidate Makefile (plain C, no meson/C++ in the
# reference tree). ngcc/drng.c and ngcc/auxfunc.c are identical to api/.
#
# NOTE (candidate side): sig_keygen does NOT draw from drng_algorithm. It seeds
# a private DRNG_ctx either from a seed installed through the non-API helper
# galas_set_kat_seed() (called only by the candidate's own ngcc/kat_gen.c) or,
# by default, from 32 zero bytes. The official KAT_SIG.c flow (seed
# drng_algorithm, call sig_keygen) therefore yields different keys and the
# submitted KATs cannot be reproduced through the API -> MISMATCH (PK differs
# from record 0 on; sign itself is deterministic). Verified out of tree: with
# keygen drawing k,x from drng_algorithm instead, records 0-2 of
# KAT_SIG_Galas-160S.txt reproduce exactly, so only the RNG plumbing deviates.
# Left unpatched: this is a candidate-side API deviation, not a build problem.
GALAS_SRCS := galas/gf2n.c galas/owf.c galas/bf.c galas/xof.c galas/bavc.c galas/vole.c \
              galas/instances.c galas/universal_hashing.c galas/vole_check_ref.c \
              galas/qs_proof_ref.c galas/quicksilver.c galas/qs_gadgets.c galas/random_oracle.c \
              galas/sign_helpers.c galas/challenge.c galas/qs_constraint.c \
              galas/qs_full_constraints.c ngcc/auxfunc.c ngcc/drng.c ngcc/SIG_AlgorithmInstance.c

# label (public name), GALAS_INSTANCE macro
define GALAS
SRCS_$(1) := $(GALAS_SRCS)
INC_$(1)  := -Isrc/$(1)/galas -Isrc/$(1)/ngcc
DEFS_$(1) := -DGALAS_INSTANCE=$(2) -DGALAS_INSTANCE_NAME='"$(1)"'
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"ngcc/SIG_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call GALAS,Galas-160S,GALAS_160S))
$(eval $(call GALAS,Galas-160F,GALAS_160F))
$(eval $(call GALAS,Galas-256S,GALAS_256S))
$(eval $(call GALAS,Galas-256F,GALAS_256F))
$(eval $(call GALAS,Galas-384S,GALAS_384S))
$(eval $(call GALAS,Galas-384F,GALAS_384F))
$(eval $(call GALAS,Galas-512S,GALAS_512S))
$(eval $(call GALAS,Galas-512F,GALAS_512F))

include ../api/link_finish.mk

build/reproduce_long_message: reproduce_long_message.c | build/Galas-160F
	$(CC) -O2 -std=gnu11 -Wall -Wextra -o $@ $< -ldl

reproduce-long-message: build/reproduce_long_message \
                        lib/libGalas-160F.so lib/libGalas-256F.so
	./build/reproduce_long_message ./lib/libGalas-160F.so
	./build/reproduce_long_message ./lib/libGalas-256F.so

.PHONY: reproduce-long-message
