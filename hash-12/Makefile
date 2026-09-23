# NGCC uniform build: hash-12 Iphe (reference implementation)
NGCC_ID   := hash-12
NGCC_TYPE := hash
NGCC_ALG  := Iphe
include ../api/link_rules.mk

REF := Iphe/Implementations/Reference_Implementation
NGCC_KAT_DIR := Iphe/Test_Vectors

# Per the instance CMakeLists: library = CryptHash_AlgorithmInstance.c + drng.c;
# iphe_selftest.c and KAT_CryptHash.c are executables and are excluded.
define IPHE
SRCS_$(1) := CryptHash_AlgorithmInstance.c drng.c
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call IPHE,Iphe-512))
$(eval $(call IPHE,Iphe-768))
$(eval $(call IPHE,Iphe-1024))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libIphe-512.so lib/libIphe-1024.so
	python3 reproduce_cross_domain.py
