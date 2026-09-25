# NGCC uniform build: sign-33 VDOO (reference implementation)
NGCC_ID   := sign-33
NGCC_TYPE := sig
NGCC_ALG  := VDOO
include ../api/link_rules.mk

# Note: the top-level directory is "Implementation" (singular) in this
# submission.
REF := Implementation/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list = LIB_SOURCES of each instance's CMakeLists.txt; the parameter
# set is selected with -DVDOO_<n> (vdoo_config.h has no default). The default
# glob is not usable because vdoo-keygen.c / vdoo-sign.c / vdoo-verif.c are
# command-line tools with their own main(). drng.c/auxfunc.c are the
# instance's own copies (identical to api/).
#
# Randomness: rng.c defines its OWN context `DRNG_ctx drng` (not
# drng_algorithm) which the ICCS KAT driver never seeds; keygen/sign draw
# from it via get_randombytes(). SIG_AlgorithmInstance.c declares
# `extern DRNG_ctx drng_algorithm` but never reads from it. We build exactly
# what the candidate ships. Consequence: all key/signature randomness comes
# from a zero-initialised, never-seeded SM3-DRNG state (deterministic, with
# a running counter), so the KAT output does not depend on the per-count
# Seed at all. The submitted KAT files were generated the same way, so the
# harness reports PASS, but this is a suspected candidate bug.
VDOO_SRCS := auxfunc.c blas_comm.c drng.c parallel_matrix_op.c rng.c utils.c api.c \
             SIG_AlgorithmInstance.c vdoo_keypair.c vdoo_sign.c vdoo_verif.c

define VDOO
SRCS_$(1) := $(VDOO_SRCS)
DEFS_$(1) := -DVDOO_$(2)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call VDOO,vdoo_128,128))
$(eval $(call VDOO,vdoo_256,256))
$(eval $(call VDOO,vdoo_512,512))

REPLAY_SOURCE := ../security/vdoo_public_forgery_replay.c
REPLAY_REF := $(REF)/vdoo_128
REPLAY_INPUTS := $(addprefix $(REPLAY_REF)/,$(VDOO_SRCS))

bin:
	mkdir -p $@

bin/replay_public_forgery: $(REPLAY_SOURCE) $(REPLAY_INPUTS) | bin
	$(CC) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -DVDOO_128 \
		-I$(REPLAY_REF) -o $@ $^ -lcrypto

.PHONY: replay-forgery
replay-forgery: bin/replay_public_forgery

.PHONY: reproduce-posted-signature
reproduce-posted-signature: replay-forgery
	bin/replay_public_forgery

include ../api/link_finish.mk

.PHONY: clean-witness
clean: clean-witness
clean-witness:
	rm -rf bin
