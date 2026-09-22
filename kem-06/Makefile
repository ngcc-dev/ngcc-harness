# NGCC uniform build: kem-06 BRA (reference implementation)
NGCC_ID   := kem-06
NGCC_TYPE := kem
NGCC_ALG  := BRA
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Object list from each instance's Makefile (BRA_OBJS + KEM_BRA-<n>.o +
# LIB_OBJS): src/*.c, five of the src/rbc-<m>/ files (m differs per instance),
# lib/random_source/random_source.c (maps the rbc random source onto
# drng_algorithm), the portable XKCP opt64 Keccak (lib/XKCP/avx2 is not used
# by the reference build) and the official drng.c/auxfunc.c (byte-identical to
# api/). The API header is named KEM_BRA-<n>.h rather than
# KEM_AlgorithmInstance.h. main_bra.c (demo, defines drng_algorithm and reads
# /dev/urandom) and KAT_KEM.c are excluded; the shim provides drng_algorithm.
BRA_SRCS := src/augmented_gabidulin.c src/bra.c src/kem.c src/parsing.c src/qpoly.c \
            lib/random_source/random_source.c \
            lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c \
            lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c \
            lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c \
            drng.c auxfunc.c

# label, rbc-m
define BRA
SRCS_$(1) := $(BRA_SRCS) KEM_$(1).c $(addprefix src/rbc-$(2)/,rbc_elt.c rbc_vec.c rbc_poly.c rbc_qre.c rbc_vspace.c)
INC_$(1)  := -Isrc/$(1)/src -Isrc/$(1)/src/rbc-$(2) -Isrc/$(1)/lib/random_source \
             -Isrc/$(1)/lib/XKCP -Isrc/$(1)/lib/XKCP/opt64 -Isrc/$(1)/lib/wrapper
CFLAGS_$(1) := -std=c99
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"KEM_$(1).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call BRA,BRA-128,67))
$(eval $(call BRA,BRA-256,83))
$(eval $(call BRA,BRA-512,127))

include ../api/link_finish.mk

# Candidate-local ASan witness for kem-06-1.  It links the submitted BRA-128
# reference sources directly; no candidate source is patched.
BRA128 := $(REF)/BRA-128
BRA128_REPRO_SRCS := reproduce_decoder_overflow.c \
                    $(addprefix $(BRA128)/,$(BRA_SRCS)) \
                    $(BRA128)/KEM_BRA-128.c \
                    $(addprefix $(BRA128)/src/rbc-67/,rbc_elt.c rbc_vec.c rbc_poly.c rbc_qre.c rbc_vspace.c)
BRA128_REPRO_INC := -I$(BRA128) -I$(BRA128)/src -I$(BRA128)/src/rbc-67 \
                    -I$(BRA128)/lib/random_source -I$(BRA128)/lib/XKCP \
                    -I$(BRA128)/lib/XKCP/opt64 -I$(BRA128)/lib/wrapper
REPRO_CC ?= /usr/bin/clang
REPRO_SAN_FLAGS ?= -O1 -g -std=c99 -fsanitize=address -fno-omit-frame-pointer \
                   -fsanitize-ignorelist=reproduce_decoder_overflow.ignore

reproduce_decoder_overflow: $(BRA128_REPRO_SRCS) reproduce_decoder_overflow.ignore
	$(REPRO_CC) $(REPRO_SAN_FLAGS) $(BRA128_REPRO_INC) -o $@ $(BRA128_REPRO_SRCS) -lm

.PHONY: exploit clean-exploit
exploit: reproduce_decoder_overflow
	ASAN_OPTIONS=detect_leaks=0 ./reproduce_decoder_overflow --honest
	@set -eu; tmp=$$(mktemp); trap 'rm -f "$$tmp"' EXIT; \
	  set +e; ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 ./reproduce_decoder_overflow --mutated >"$$tmp" 2>&1; rc=$$?; set -e; \
	  cat "$$tmp"; test $$rc -ne 0; \
	  grep -q 'AddressSanitizer: heap-buffer-overflow' "$$tmp"; \
	  grep -q 'qpoly.c:458' "$$tmp"; \
	  echo 'CONFIRMED kem-06-1: ASan caught the unchecked q-polynomial bound'

clean: clean-exploit
clean-exploit:
	rm -f reproduce_decoder_overflow reproduce_field_oob_67 reproduce_field_oob_83 reproduce_field_oob_127

# Independent guard-page check for kem-06-2.  The 127-bit field is the
# negative control: its submitted reference allocation has the fourth limb.
define FIELD_REPRO
reproduce_field_oob_$(1): reproduce_field_oob.c \
                          $(REF)/BRA-$(2)/src/rbc-$(1)/rbc_elt.c \
                          $(REF)/BRA-$(2)/lib/random_source/random_source.c \
                          $(REF)/BRA-$(2)/drng.c $(REF)/BRA-$(2)/auxfunc.c
	$(CC) -O2 -g -std=c99 -DBRA_FIELD=$(1) \
	  -I$(REF)/BRA-$(2) -I$(REF)/BRA-$(2)/src/rbc-$(1) \
	  -I$(REF)/BRA-$(2)/lib/random_source -I$(REF)/BRA-$(2)/lib/XKCP \
	  -o $$@ $$^ -lm
endef

$(eval $(call FIELD_REPRO,67,128))
$(eval $(call FIELD_REPRO,83,256))
$(eval $(call FIELD_REPRO,127,512))

.PHONY: exploit-field
exploit-field: reproduce_field_oob_67 reproduce_field_oob_83 reproduce_field_oob_127
	./reproduce_field_oob_67
	./reproduce_field_oob_83
	./reproduce_field_oob_127
