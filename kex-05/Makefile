# NGCC uniform build: kex-05 Loom (LoomKEX, reference implementation)
NGCC_ID   := kex-05
NGCC_TYPE := kex
NGCC_ALG  := Loom
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# Source lists from the CMakeLists.txt of each LoomKEX-N directory: the KEM
# (kem/, "kem1"), the signature core (sig/, "sig1", with the per-level NTT
# table set sig/ntt/<qset>/ntt_ref.c) and the KAT_KEX_LoomKEX-N target's
# protocol sources (loom/). -DLOOM_MODE=1|3|5 selects the parameter set.
# The KAT path uses drng.c + symmetric-iccs.c; loom/loom.c defines
# randombytes() on top of drng_algorithm. loom/rng.c (NIST AES-CTR DRBG on
# OpenSSL), loom/fips202.c + symmetric-shake.c (LOOM_USE_SHAKE variant) and
# sig/SIG_AlgorithmInstance.c are not part of the KAT build and are not
# compiled, so no -lcrypto. The API header is loom/KEX_LoomKEX-N.h.
# kem/ and sig/ have colliding header basenames (poly.h, ntt.h, ...); each
# file's own directory is searched first, and the -I order mirrors CMake.
LOOM_KEM := cbd.c indcpa.c kem_cpaf.c msgenc.c ntt.c poly.c poly_invq.c polyvec.c reduce.c verify.c
LOOM_SIG := rounding.c sign.c polyvec.c sampler.c sampler_u.c irs.c packing.c poly.c poly_ntt.c \
            reduce.c rans.c approx_exp.c approx_log.c
LOOM_PROTO := loom.c prf_mac.c state_serialize.c drng.c auxfunc.c symmetric-iccs.c

# label, LOOM_MODE, sig NTT set dir
define LOOM
SRCS_$(1) := $(addprefix loom/,$(LOOM_PROTO) KEX_$(1).c) $(addprefix kem/,$(LOOM_KEM)) \
             $(addprefix sig/,$(LOOM_SIG)) sig/ntt/$(3)/ntt_ref.c
INC_$(1)  := -Isrc/$(1)/loom -Isrc/$(1)/kem -Isrc/$(1)/sig
DEFS_$(1) := -DLOOM_MODE=$(2)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"loom/KEX_$(1).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call LOOM,LoomKEX-128,1,q15361n256))
$(eval $(call LOOM,LoomKEX-256,3,q61441n512))
$(eval $(call LOOM,LoomKEX-512,5,q59393n1024))

include ../api/link_finish.mk

.PHONY: loom-witness-build replay exploit clean-replay
# A single shared target serializes CMake's configure/build phases even when
# callers request both witnesses with make -j.
loom-witness-build:
	cmake -S ../security/loom_reference -B ../security/loom_reference/build -DCMAKE_BUILD_TYPE=Release
	cmake --build ../security/loom_reference/build --target loom_failure_replay_256 loom_state_rollback_key_recovery_256

replay: loom-witness-build
	cp ../security/loom_reference/build/bin/loom_failure_replay_256 reproduce_failure

exploit: loom-witness-build
	cp ../security/loom_reference/build/bin/loom_state_rollback_key_recovery_256 reproduce_state_rollback_key_recovery

clean: clean-replay
clean-replay:
	rm -rf ../security/loom_reference/build reproduce_failure reproduce_state_rollback_key_recovery
