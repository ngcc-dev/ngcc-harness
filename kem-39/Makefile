# NGCC uniform build: kem-39 Weaver (reference implementation)
NGCC_ID   := kem-39
NGCC_TYPE := kem
NGCC_ALG  := Weaver
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list per each instance's CMakeLists.txt (SOURCES) minus the KAT_KEM.c
# driver; parameter set selected with -DWEAVER_MODE=N. The API lives in
# KEM_WeaverKEM-NNN.c (not KEM_AlgorithmInstance.c); randombytes() is defined
# in kem_cca.c over drng_algorithm (SM3 path). rng.c (OpenSSL AES-CTR DRBG,
# only used with -DWEAVER_USE_SHAKE), symmetric-shake.c, fips202.c,
# correct_test.c and gen_invq_table.c are not part of the reference build.
# The optimized tree wants OpenSSL; the reference tree does not (nor GMP).
WV_SRCS := drng.c auxfunc.c symmetric-iccs.c kem_cca.c indcpa.c polyvec.c poly.c ntt.c cbd.c \
           reduce.c verify.c msgenc.c bch_high.c bch_low.c poly_invq.c

# label, WEAVER_MODE
define WV
SRCS_$(1) := $(WV_SRCS) KEM_$(1).c
DEFS_$(1) := -DWEAVER_MODE=$(2)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"KEM_$(1).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call WV,WeaverKEM-128,1))
$(eval $(call WV,WeaverKEM-256,3))
$(eval $(call WV,WeaverKEM-512,5))

include ../api/link_finish.mk

.PHONY: exploit-bch
exploit-bch: build/reproduce_high_bch_omission
	build/reproduce_high_bch_omission

build/reproduce_high_bch_omission: reproduce_high_bch_omission.c $(addprefix $(REF)/WeaverKEM-256/,msgenc.c bch_high.c bch_low.c reduce.c)
	@mkdir -p build
	$(CC) -O2 -DWEAVER_MODE=3 -I$(REF)/WeaverKEM-256 $(filter %.c,$^) -o $@
