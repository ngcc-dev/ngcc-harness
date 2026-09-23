# NGCC uniform build: hash-31 ZC-DMC (reference implementation)
NGCC_ID   := hash-31
NGCC_TYPE := hash
NGCC_ALG  := ZC-DMC
include ../api/link_rules.mk

REF := ZC-DMC/Implementations/Implementations/Reference_Implementation
NGCC_KAT_DIR := ZC-DMC/Test_Vectors

# Per each instance's batch_usage.txt: the instance dir (CryptHash_AlgorithmInstance.c,
# unmodified official drng.c) plus the portable ZuD permutation from the shared
# lib/ tree: lib/low/ZuD-<W>/plain/ZuD<W>-plain.c, which needs lib/common for
# brg_endian.h. CryptHash_AlgorithmInstance.c reaches the ZuD header through a
# relative "../../../lib/..." include; SRCS_ uses the same physical path since
# ".." in src/<label>/... is resolved through the symlink by the kernel.
# The 32bit/ and AVX2/ backends are not used.
ZC_LIB := $(REF)/../../lib

# label, ZuD width
define ZC
SRCS_$(1) := CryptHash_AlgorithmInstance.c drng.c ../../../lib/low/ZuD-$(2)/plain/ZuD$(2)-plain.c
INC_$(1)  := -I$(ZC_LIB)/common
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call ZC,ZC-DMC-1280-512,1280))
$(eval $(call ZC,ZC-DMC-1280-768,1280))
$(eval $(call ZC,ZC-DMC-1280-1024,1280))
$(eval $(call ZC,ZC-DMC-1536-512,1536))
$(eval $(call ZC,ZC-DMC-1536-768,1536))
$(eval $(call ZC,ZC-DMC-1536-1024,1536))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libZC-DMC-1536-512.so lib/libZC-DMC-1536-768.so
	python3 reproduce_cross_domain.py
