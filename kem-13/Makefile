# NGCC uniform build: kem-13 DKEM (reference implementation)
NGCC_ID   := kem-13
NGCC_TYPE := kem
NGCC_ALG  := DKEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list and defines from each instance's build.sh (-DDKE_FORCE_SCALAR
# -DDKE_MODE=<n> -DDKE_HASH=0 -DDKE_RANDOM=0 -fcommon). With DKE_RANDOM=0 the
# scheme draws only from drng_algorithm (KEM_AlgorithmInstance.c), so
# randombytes.c / randombytes_sys.c (getrandom(2) OS entropy) are not linked.
# KEM_AlgorithmInstance.c defines drng_algorithm itself (the official KAT
# driver relies on -fcommon to merge the two tentative definitions), hence
# -DNGCC_NO_DRNG. ALGORITHM_INSTANCE comes from parameters.h via DKE_MODE.
# drng.c/auxfunc.c are the unmodified API_PKC copies.
DKEM_SRCS := sm3.c dke_sm3.c dke_hash.c reduce.c ntt.c poly.c polyvec.c random_sampling.c \
             dke_utils.c packing.c verify.c dkecpa.c dkecca.c KEM_AlgorithmInstance.c drng.c auxfunc.c

define DKEM
SRCS_DKEM-$(1) := $(DKEM_SRCS)
DEFS_DKEM-$(1) := -DDKE_FORCE_SCALAR -DDKE_MODE=$(1) -DDKE_HASH=0 -DDKE_RANDOM=0
CFLAGS_DKEM-$(1) := -std=c99
SHIMDEFS_DKEM-$(1) := -DNGCC_NO_DRNG
$$(eval $$(call ngcc_instance,DKEM-$(1),$(REF)/DKEM-$(1)))
endef

$(eval $(call DKEM,128))
$(eval $(call DKEM,256))
$(eval $(call DKEM,512))

include ../api/link_finish.mk

.PHONY: reproduce reproduce-rejection-binding reproduce-malicious-key reproduce-ntt-overflow reproduce-truncated
reproduce: reproduce-rejection-binding reproduce-malicious-key reproduce-ntt-overflow reproduce-truncated

reproduce-rejection-binding: libs
	python3 reproduce_rejection_binding.py $(LIBS)

reproduce-malicious-key: libs
	python3 reproduce_malicious_key.py $(LIBS)

reproduce-ntt-overflow: libs
	python3 reproduce_ntt_overflow.py $(LIBS)

DKEM128_DIR := $(REF)/DKEM-128
DKEM128_REPRO_SRCS := $(addprefix $(DKEM128_DIR)/,$(DKEM_SRCS))

build/reproduce_truncated: reproduce_truncated.c $(DKEM128_REPRO_SRCS)
	@mkdir -p build
	$(CC) -O1 -g -std=c99 -fcommon -fsanitize=address -fno-omit-frame-pointer \
		-DDKE_FORCE_SCALAR -DDKE_MODE=128 -DDKE_HASH=0 -DDKE_RANDOM=0 \
		-I$(DKEM128_DIR) -o $@ $^

reproduce-truncated: build/reproduce_truncated
	@set +e; out=$$(ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 ./build/reproduce_truncated 2>&1); rc=$$?; set -e; \
		printf '%s\n' "$$out"; test $$rc -ne 0; \
		printf '%s\n' "$$out" | grep -Eq 'AddressSanitizer: (heap-buffer-overflow|unknown-crash)'; \
		printf '%s\n' "$$out" | grep -q 'READ of size'; \
		echo 'ATTACK kem-13-4 CONFIRMED one-byte-short ciphertext causes an out-of-bounds read'
