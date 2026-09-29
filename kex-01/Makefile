# NGCC uniform build: kex-01 ADKEX (reference implementation)
NGCC_ID   := kex-01
NGCC_TYPE := kex
NGCC_ALG  := ADKEX
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list and defines from each instance's build.sh (portable scalar C:
# -DADKEX_MODE=N -DDKE_MODE=N -DDKE_HASH=0 -DDKE_RANDOM=0 -DDKE_FORCE_SCALAR).
# With DKE_RANDOM=0 randombytes.c compiles to nothing; all randomness comes
# from drng_algorithm in KEX_AlgorithmInstance.c. drng.c/auxfunc.c in the
# tree are byte-identical to the official api/ copies. -fcommon as in build.sh.
ADKEX_SRCS := sm3.c dke_sm3.c dke_hash.c reduce.c ntt.c poly.c polyvec.c random_sampling.c \
              dke_utils.c packing.c verify.c dkecpa.c dkecca.c drng.c auxfunc.c \
              adkex_derand.c KEX_AlgorithmInstance.c

# label, mode
define ADKEX
SRCS_$(1) := $(ADKEX_SRCS)
DEFS_$(1) := -DADKEX_MODE=$(2) -DDKE_MODE=$(2) -DDKE_HASH=0 -DDKE_RANDOM=0 -DDKE_FORCE_SCALAR
CFLAGS_$(1) := -fcommon
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call ADKEX,ADKEX-128,128))
$(eval $(call ADKEX,ADKEX-256,256))
$(eval $(call ADKEX,ADKEX-512,512))

include ../api/link_finish.mk

.PHONY: reproduce-correctness reproduce-truncated clean-reproducers

reproduce-correctness:
	python3 reproduce_correctness.py

reproduce-truncated:
	$(CC) -O1 -g -fsanitize=address -fno-omit-frame-pointer -fcommon \
		-DADKEX_MODE=128 -DDKE_MODE=128 -DDKE_HASH=0 -DDKE_RANDOM=0 \
		-DDKE_FORCE_SCALAR -I$(REF)/ADKEX-128 \
		-o reproduce_truncated_asan reproduce_truncated.c \
		$(addprefix $(REF)/ADKEX-128/,$(ADKEX_SRCS))
	@set +e; out="$$(ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
		./reproduce_truncated_asan 2>&1)"; rc=$$?; set -e; \
		printf '%s\n' "$$out"; test $$rc -ne 0; \
		printf '%s\n' "$$out" | grep -q 'heap-buffer-overflow'; \
		echo 'CONFIRMED kex-01-3 ADKEX ignores incoming message lengths'

clean-reproducers:
	$(RM) reproduce_truncated_asan
