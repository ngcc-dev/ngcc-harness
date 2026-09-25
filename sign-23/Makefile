# NGCC uniform build: sign-23 Shuttle (reference implementation)
NGCC_ID   := sign-23
NGCC_TYPE := sig
NGCC_ALG  := Shuttle
include ../api/link_rules.mk

REF := Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Test_Vectors

# Source list = SCHEME_SRCS of the instance Makefile (-DSHUTTLE_MODE=<n>,
# -Intt/<qset>); the NTT lives in ntt/<qset>/ntt_ref.c. fips202.c is only
# used under SHA3_MODE and is not compiled (default NGCC_MODE: SM3 Hash-DRBG
# XOF via auxfunc). Randomness is drawn from drng_algorithm in
# SIG_AlgorithmInstance.c. Shipped drng.c/auxfunc.c are the official ones.
SHUTTLE_SRCS := SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c \
  rounding.c packing.c poly.c poly_ntt.c reduce.c rans.c approx_exp.c \
  approx_log.c symmetric.c drng.c auxfunc.c

# label, mode, qset
define SHUTTLE
SRCS_$(1) := $(SHUTTLE_SRCS) ntt/$(3)/ntt_ref.c
INC_$(1)  := -Isrc/$(1)/ntt/$(3)
DEFS_$(1) := -DSHUTTLE_MODE=$(2)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call SHUTTLE,SHUTTLE-128,128,q15361n256))
$(eval $(call SHUTTLE,SHUTTLE-256,256,q61441n512))
$(eval $(call SHUTTLE,SHUTTLE-512,512,q59393n1024))

bin:
	mkdir -p $@

define shuttle_recovery_target
bin/reproduce_covariance_$(1): ../security/shuttle_covariance_recovery.c \
  $(addprefix src/SHUTTLE-$(1)/,$(SHUTTLE_SRCS)) \
  src/SHUTTLE-$(1)/ntt/$(2)/ntt_ref.c | bin
	$(CC) -O3 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -fwrapv \
		-fno-strict-aliasing -DSHUTTLE_MODE=$(1) -Isrc/SHUTTLE-$(1) \
		-Isrc/SHUTTLE-$(1)/ntt/$(2) -o $$@ $$^ -lm
endef
$(eval $(call shuttle_recovery_target,128,q15361n256))
$(eval $(call shuttle_recovery_target,256,q61441n512))
$(eval $(call shuttle_recovery_target,512,q59393n1024))

.PHONY: exploit
exploit: bin/reproduce_covariance_128 bin/reproduce_covariance_256 bin/reproduce_covariance_512

.PHONY: reproduce
reproduce: exploit
	@for level in 128 256 512; do bin/reproduce_covariance_$$level 8 || exit; done

include ../api/link_finish.mk

.PHONY: clean-witness
clean: clean-witness
clean-witness:
	rm -rf bin
