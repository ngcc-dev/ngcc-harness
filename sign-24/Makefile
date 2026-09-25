# NGCC uniform build: sign-24 Sigurd (reference implementation)
NGCC_ID   := sign-24
NGCC_TYPE := sig
NGCC_ALG  := Sigurd
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Implementations/Makefile (REF_SIG*_SRCS): SIG_AlgorithmInstance.c sig_core.c
# rsencode.c rsencode_common.c auxfunc.c drng.c, no defines; the default glob
# gives exactly that. Randomness comes from drng_algorithm (sig_core.c and
# SIG_AlgorithmInstance.c). Shipped drng.c/auxfunc.c are the official ones.
# ALGORITHM_INSTANCE is "Sigurd<n>_REF" and the KAT files are named
# KAT_SIG_Sigurd<n>_REF.txt (the _OPT files belong to the optimized tree).
$(eval $(call ngcc_instance,Sigurd-128,$(REF)/Sigurd-128))
$(eval $(call ngcc_instance,Sigurd-256,$(REF)/Sigurd-256))
$(eval $(call ngcc_instance,Sigurd-512,$(REF)/Sigurd-512))

# A public-signature-only recovery driver, compiled against the submitted
# reference sources so it can observe the verifier's public opening indices and
# call the prover with the recovered witness instead of a secret-key seed.
RECOVERY_DRIVER := ../security/sigurd_chunk_recovery.c
RECOVERY_SRCS := SIG_AlgorithmInstance.c sig_core.c rsencode.c rsencode_common.c auxfunc.c drng.c

define sigurd_recovery_target
bin/reproduce_recovery_$(1): $(RECOVERY_DRIVER) $(addprefix $(REF)/Sigurd-$(1)/,$(RECOVERY_SRCS)) | bin
	$(CC) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter \
		$(if $(filter 256,$(1)),-DSIGURD_LEVEL_256,) -I$(REF)/Sigurd-$(1) \
		-o $$@ $$^
endef
$(foreach level,128 256 512,$(eval $(call sigurd_recovery_target,$(level))))

bin:
	mkdir -p $@

.PHONY: exploit
exploit: bin/reproduce_recovery_128 bin/reproduce_recovery_256 bin/reproduce_recovery_512

.PHONY: reproduce
reproduce: exploit
	@for level in 128 256 512; do \
	  for seed_id in 0 1; do bin/reproduce_recovery_$$level $$seed_id || exit; done; \
	done

include ../api/link_finish.mk

.PHONY: clean-witness
clean: clean-witness
clean-witness:
	rm -rf bin
