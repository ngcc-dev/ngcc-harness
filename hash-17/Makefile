# NGCC uniform build: hash-17 MasterCube (reference implementation)
NGCC_ID   := hash-17
NGCC_TYPE := hash
NGCC_ALG  := MasterCube
include ../api/link_rules.mk

REF := MasterCube/Implementations/Reference_Implementation
NGCC_KAT_DIR := MasterCube/Test_Vectors

# Per Implementations/CMakeLists.txt (MCUBE_IMPL=reference): KAT_CryptHash.c
# CryptHash_AlgorithmInstance.c drng.c MasterCube-NNN.c, -std=c99 -O2
# (default glob minus the KAT driver).
$(eval $(call ngcc_instance,MasterCube-512,$(REF)/MasterCube-512))
$(eval $(call ngcc_instance,MasterCube-768,$(REF)/MasterCube-768))
$(eval $(call ngcc_instance,MasterCube-1024,$(REF)/MasterCube-1024))

include ../api/link_finish.mk

.PHONY: reproduce-inverse
reproduce-inverse: reproduce_inverse
	./reproduce_inverse

reproduce_inverse: reproduce_inverse.c
	$(CC) -O2 -std=c99 -I. $< -o $@
