# NGCC uniform build: kem-07 BRQC (reference implementation)
NGCC_ID   := kem-07
NGCC_TYPE := kem
NGCC_ALG  := BRQC
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Same layout as kem-06 (BRA). Object list from each instance's Makefile
# (BRQC_OBJS + KEM_BRQC-<n>.o + LIB_OBJS): src/*.c, five of the src/rbc-<m>/
# files (m differs per instance), lib/random_source/random_source.c (maps the
# rbc random source onto drng_algorithm), the portable XKCP opt64 Keccak
# (lib/XKCP/avx2 is not used by the reference build) and the official
# drng.c/auxfunc.c (byte-identical to api/). The API header is KEM_BRQC-<n>.h.
# main_brqc.c (demo, defines drng_algorithm and reads /dev/urandom) and
# KAT_KEM.c are excluded; the shim provides drng_algorithm.
BRQC_SRCS := src/brqc.c src/gabidulin.c src/kem.c src/parsing.c src/qpoly.c \
             lib/random_source/random_source.c \
             lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c \
             lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c \
             lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c \
             drng.c auxfunc.c

# label, rbc-m
define BRQC
SRCS_$(1) := $(BRQC_SRCS) KEM_$(1).c $(addprefix src/rbc-$(2)/,rbc_elt.c rbc_vec.c rbc_poly.c rbc_qre.c rbc_vspace.c)
INC_$(1)  := -Isrc/$(1)/src -Isrc/$(1)/src/rbc-$(2) -Isrc/$(1)/lib/random_source \
             -Isrc/$(1)/lib/XKCP -Isrc/$(1)/lib/XKCP/opt64 -Isrc/$(1)/lib/wrapper
CFLAGS_$(1) := -std=c99
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"KEM_$(1).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call BRQC,BRQC-128,127))
$(eval $(call BRQC,BRQC-256,163))
$(eval $(call BRQC,BRQC-512,229))

include ../api/link_finish.mk
