# NGCC uniform build: kem-10 C-Multi-UR-AG (reference implementation)
NGCC_ID   := kem-10
NGCC_TYPE := kem
NGCC_ALG  := C-Multi-UR-AG
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from each instance's Makefile (CMULTIURAG_OBJS + LIB_OBJS): the
# core lives in src/ and src/rbc-<m>/, hashing in lib/XKCP (portable opt64
# permutation, avx2/ not used). lib/random_source/random_source.c draws from
# the shared drng_algorithm (no OS entropy in the library). drng.c/auxfunc.c
# are the unmodified API_PKC copies. The API header is KEM_CMultiURAG-<n>.h.
CMU_SRCS = KEM_CMultiURAG-$(1).c drng.c auxfunc.c lib/random_source/random_source.c \
            lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c \
            lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c \
            lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c \
            src/rbc-$(2)/rbc_elt.c src/rbc-$(2)/rbc_vec.c src/rbc-$(2)/rbc_poly.c \
            src/rbc-$(2)/rbc_qre.c src/rbc-$(2)/rbc_vspace.c src/rbc-$(2)/rbc_mat.c \
            src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/cmultiurag.c src/kem.c

# CMU_SRCS is a function of (level, rbc field size); label suffix, rbc size
define CMU
SRCS_CMultiURAG-$(1) := $$(call CMU_SRCS,$(1),$(2))
INC_CMultiURAG-$(1)  := -Isrc/CMultiURAG-$(1)/src -Isrc/CMultiURAG-$(1)/src/rbc-$(2) \
    -Isrc/CMultiURAG-$(1)/lib/random_source -Isrc/CMultiURAG-$(1)/lib/XKCP \
    -Isrc/CMultiURAG-$(1)/lib/XKCP/opt64 -Isrc/CMultiURAG-$(1)/lib/wrapper
CFLAGS_CMultiURAG-$(1) := -std=c99
SHIMDEFS_CMultiURAG-$(1) := -DNGCC_INSTANCE_HEADER='"KEM_CMultiURAG-$(1).h"'
$$(eval $$(call ngcc_instance,CMultiURAG-$(1),$(REF)/CMultiURAG-$(1)))
endef

$(eval $(call CMU,128,79))
$(eval $(call CMU,256,127))
$(eval $(call CMU,512,181))

include ../api/link_finish.mk
