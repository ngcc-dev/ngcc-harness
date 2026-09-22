# NGCC uniform build: kem-06 BRA (reference implementation, BRA-128)
# The reference sources include x86intrin.h, so this instance is x86_64.
NGCC_ID   := kem-06
NGCC_TYPE := kem
NGCC_ALG  := BRA
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation

# Object list from the submission's BRA-128 link line. main_bra.c and
# KAT_KEM.c define main (and drng_algorithm); the shim provides drng_algorithm.
# The archive ships KAT_KEM.c but no KAT text. kat.sha256 is the harness
# digest of that driver's 10-count output (KAT_KEM_BRA-128.txt).
BRA_SRCS := \
	src/rbc-67/rbc_elt.c src/rbc-67/rbc_vec.c src/rbc-67/rbc_poly.c \
	src/rbc-67/rbc_qre.c src/rbc-67/rbc_vspace.c \
	src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/bra.c src/kem.c \
	KEM_BRA-128.c drng.c auxfunc.c \
	lib/random_source/random_source.c \
	lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c \
	lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c \
	lib/XKCP/SimpleFIPS202.c \
	lib/XKCP/opt64/KeccakP-1600-opt64.c \
	lib/XKCP/opt64/KeccakP-1600-times4-on1.c

SRCS_BRA-128 := $(BRA_SRCS)
INC_BRA-128 := -Isrc/BRA-128/src -Isrc/BRA-128/src/rbc-67 \
	-Isrc/BRA-128/lib/random_source -Isrc/BRA-128/lib/XKCP \
	-Isrc/BRA-128/lib/XKCP/opt64 -Isrc/BRA-128/lib/wrapper
SHIMDEFS_BRA-128 := -DNGCC_INSTANCE_HEADER='"KEM_BRA-128.h"'

$(eval $(call ngcc_instance,BRA-128,$(REF)/BRA-128))

include ../api/link_finish.mk
