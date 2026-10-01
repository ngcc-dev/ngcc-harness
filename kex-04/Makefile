# NGCC uniform build: kex-04 DKEX (reference implementation)
NGCC_ID   := kex-04
NGCC_TYPE := kex
NGCC_ALG  := DKEX
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source lists and defines from each instance's build.sh (minus KAT_KEX.c):
# the DKE core (SM3 hash, scalar paths), the vendored pq-crystals ML-DSA in
# dilithium/ and the DKEX layer. The three directories are identical apart
# from build.sh; the parameter set is chosen by -DADKEX_MODE=N -DDKE_MODE=N
# and the ML-DSA level by -DADKEX_SIG_MLDSA_LEVEL=L -DDILITHIUM_MODE=L
# (2/5/5). -DDKE_HASH=0 -DDKE_RANDOM=0 -DDKE_FORCE_SCALAR select SM3, the DRNG
# and the scalar reference code. All randomness comes from drng_algorithm
# (randombytes.c is a DRNG-backed replacement that also feeds the ML-DSA
# keygen seed). Three-pass protocol.
# build.sh compiles the core, dilithium/ and the layer files with separate -I
# groups because header basenames collide (ntt.h, poly.h, ...). Here one
# -I.../dilithium is added after the instance dir: quoted includes resolve in
# the including file's own directory first, so each group still gets its own
# headers. build.sh also passes -fcommon, which is not needed (links without it).
# The "ALGORITHM_INSTANCE redefined" warning is benign: parameters.h defines
# "DKE-<n>" first and KEX_AlgorithmInstance.h then sets the final "DKEX-<n>".
DKEX_SRCS := sm3.c dke_sm3.c dke_hash.c reduce.c ntt.c poly.c polyvec.c random_sampling.c \
             dke_utils.c packing.c verify.c dkecpa.c drng.c auxfunc.c \
             dilithium/fips202.c dilithium/ntt.c dilithium/packing.c dilithium/poly.c \
             dilithium/polyvec.c dilithium/reduce.c dilithium/rounding.c dilithium/sign.c \
             dilithium/symmetric-shake.c \
             adkex_derand.c adkex_sig_mldsa.c KEX_AlgorithmInstance.c randombytes.c

# label, mode, ML-DSA level
define DKEX
SRCS_$(1) := $(DKEX_SRCS)
INC_$(1)  := -Isrc/$(1)/dilithium
DEFS_$(1) := -DDKE_FORCE_SCALAR -DADKEX_MODE=$(2) -DDKE_MODE=$(2) -DDKE_HASH=0 -DDKE_RANDOM=0 \
             -DADKEX_SIG_BACKEND_MLDSA -DADKEX_SIG_MLDSA_LEVEL=$(3) -DDILITHIUM_MODE=$(3)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call DKEX,DKEX-128,128,2))
$(eval $(call DKEX,DKEX-256,256,5))
$(eval $(call DKEX,DKEX-512,512,5))

include ../api/link_finish.mk

DKEX128_DIR := $(REF)/DKEX-128
DKEX128_REPRO_SRCS := $(addprefix $(DKEX128_DIR)/,$(DKEX_SRCS))

.PHONY: reproduce-truncated
build/reproduce_truncated: reproduce_truncated.c $(DKEX128_REPRO_SRCS)
	@mkdir -p build
	$(CC) -O1 -g -fcommon -fsanitize=address -fno-omit-frame-pointer \
		-DDKE_FORCE_SCALAR -DADKEX_MODE=128 -DDKE_MODE=128 -DDKE_HASH=0 -DDKE_RANDOM=0 \
		-DADKEX_SIG_BACKEND_MLDSA -DADKEX_SIG_MLDSA_LEVEL=2 -DDILITHIUM_MODE=2 \
		-I$(DKEX128_DIR) -I$(DKEX128_DIR)/dilithium -o $@ $^

reproduce-truncated: build/reproduce_truncated
	@set +e; out=$$(ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 ./build/reproduce_truncated 2>&1); rc=$$?; set -e; \
		printf '%s\n' "$$out"; test $$rc -ne 0; \
		printf '%s\n' "$$out" | grep -q 'AddressSanitizer: heap-buffer-overflow'; \
		echo 'ATTACK kex-04-2 CONFIRMED one-byte pass-1 message causes an out-of-bounds read'
