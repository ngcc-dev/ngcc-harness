# NGCC uniform build: hash-11 Garnet (reference implementation)
NGCC_ID   := hash-11
NGCC_TYPE := hash
NGCC_ALG  := Garnet
include ../api/link_rules.mk

REF := Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation
NGCC_KAT_DIR := Garnet/Test_Vectors

# Layout anomaly: a single Reference_Implementation/Garnet directory holds all
# parameter sets. The API header is CryptHash_Garnet.h (ALGORITHM_INSTANCE
# "Garnet", DIGEST_BIT_LENGTH 512 for every build), so the instance name and
# digest size are supplied to the shim. Per README.txt each build links
# CryptHash_Garnet.c (dispatch on digest_len_bits) + one Garnet_512*.c
# (capacity variant: Garnet_512.c = capacity 1024) + Garnet_768.c + Garnet_1024.c
# + drng.c. main*.c / KAT_CryptHash.c are drivers; include/ has stale headers
# (src dir headers come first on the include path).
# Only one Garnet_1024.c is shipped; it reproduces KAT_*_Garnet_1024. The
# second 1024-bit KAT set (Garnet_1024_DM4x4, README: "Garnet-1024a", sponge-DM
# on a 4x4 state) has NO reference C source in the tree (the only DM4x4 code is
# Optimized_Implementation/Garnet_1024_DM/Garnet_1024.S, x86-64 assembly), so
# that label is built from the same sources and is expected to MISMATCH from
# message length 512 bits on.
# label, 512-variant source, digest bits, KAT name
define GARNET
SRCS_$(1) := CryptHash_Garnet.c $(2) Garnet_768.c Garnet_1024.c drng.c
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"CryptHash_Garnet.h"' -DNGCC_INSTANCE='"$(4)"' -DNGCC_DIGEST_BITS=$(3)
$$(eval $$(call ngcc_instance,$(1),$(REF)/Garnet))
endef

$(eval $(call GARNET,Garnet_512_Cap512,Garnet_512_512.c,512,Garnet_512_Cap512))
$(eval $(call GARNET,Garnet_512_Cap640,Garnet_512_640.c,512,Garnet_512_Cap640))
$(eval $(call GARNET,Garnet_512_Cap768,Garnet_512_768.c,512,Garnet_512_Cap768))
$(eval $(call GARNET,Garnet_512_Cap896,Garnet_512_896.c,512,Garnet_512_Cap896))
$(eval $(call GARNET,Garnet_512_Cap1024,Garnet_512.c,512,Garnet_512_Cap1024))
$(eval $(call GARNET,Garnet_768,Garnet_512.c,768,Garnet_768))
$(eval $(call GARNET,Garnet_1024,Garnet_512.c,1024,Garnet_1024))
$(eval $(call GARNET,Garnet_1024_DM4x4,Garnet_512.c,1024,Garnet_1024_DM4x4))

include ../api/link_finish.mk
