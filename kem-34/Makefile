# NGCC uniform build: kem-34 Rudraksh2 (reference implementation)
NGCC_ID   := kem-34
NGCC_TYPE := kem
NGCC_ALG  := Rudraksh2
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# The reference directories ship no Makefile. The NGCC API is implemented in
# KEM_lwekem<N>.c / KEM_lwekem<N>.h (header named differently from
# KEM_AlgorithmInstance.h); it uses indcpa.c poly.c polyvec.c cbd.c minal.c
# reduce.c ntt.c verify.c plus the official auxfunc.c/drng.c. api.c is an
# alternative, namespaced (kem_light_ref_*) copy of the same API that would
# clash with KEM_lwekem<N>.c, and kex.c includes a kem.h that is not shipped,
# so neither is compiled. No defines: params.h in each directory fixes the
# parameter set. Randomness comes from drng_algorithm (KEM_lwekem<N>.c,
# indcpa.c). Test_Vectors/ and Implementations/Test_Vectors/ are identical.
define RUDRAKSH
SRCS_$(1) := KEM_$(1).c indcpa.c poly.c polyvec.c cbd.c minal.c reduce.c ntt.c verify.c \
             auxfunc.c drng.c
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"KEM_$(1).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call RUDRAKSH,lwekem128))
$(eval $(call RUDRAKSH,lwekem256))
$(eval $(call RUDRAKSH,lwekem512))

include ../api/link_finish.mk

RUD128_DIR := $(REF)/lwekem128
RUD128_REPRO_SRCS := $(addprefix $(RUD128_DIR)/,$(RUD_SRCS) KEM_lwekem128.c)

.PHONY: reproduce-message-space reproduce-length-read reproduce-parameters

build/reproduce_message_space: reproduce_message_space.c $(RUD128_REPRO_SRCS)
	@mkdir -p build
	$(CC) -std=c99 -O2 -fcommon -w -I$(RUD128_DIR) -o $@ $^

reproduce-message-space: build/reproduce_message_space
	@./build/reproduce_message_space

build/reproduce_length_read: reproduce_length_read.c $(RUD128_REPRO_SRCS)
	@mkdir -p build
	$(CC) -std=c99 -O1 -g -fcommon -w -fsanitize=address -I$(RUD128_DIR) -o $@ $^

reproduce-length-read: build/reproduce_length_read
	@out=$$(ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 ./build/reproduce_length_read 2>&1; true); \
	printf '%s\n' "$$out" | grep -E 'calling kem_dec|heap-buffer-overflow|READ of size|poly_decompress|unpack_ciphertext' || true; \
	if printf '%s\n' "$$out" | grep -q 'AddressSanitizer: heap-buffer-overflow'; then \
	  echo 'ATTACK rudraksh-length-contract lwekem128 CONFIRMED one-byte-short ciphertext causes an out-of-bounds read'; \
	else \
	  echo 'ATTACK rudraksh-length-contract lwekem128 NOT-CONFIRMED AddressSanitizer did not report the expected read'; exit 1; \
	fi

reproduce-parameters:
	@python3 reproduce_parameters.py
