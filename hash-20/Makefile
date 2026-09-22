# NGCC uniform build: hash-20 Mozi Hash Function (reference implementation)
NGCC_ID   := hash-20
NGCC_TYPE := hash
NGCC_ALG  := Mozi
include ../api/link_rules.mk

REF := Mozi/Implementations/Reference_Implementation
NGCC_KAT_DIR := Mozi/Test_Vectors/Test_Vector

# Plain instances: CryptHash_AlgorithmInstance.c + drng.c (identical to
# api/drng.c), default glob.
$(eval $(call ngcc_instance,MOZI-384,$(REF)/MOZI-384))
$(eval $(call ngcc_instance,MOZI-512,$(REF)/MOZI-512))
$(eval $(call ngcc_instance,MOZI-768,$(REF)/MOZI-768))
$(eval $(call ngcc_instance,MOZI-1024,$(REF)/MOZI-1024))

# XOF instances: the submitted KATs cover output lengths 256/1024/2048 bits
# (KAT_*_MOZI-384-XOF-<len>.txt) but the shipped header only carries the 2048
# configuration (ALGORITHM_INSTANCE "MOZI-384-XOF-2048", DIGEST_BIT_LENGTH
# 2048). CryptHash() takes the digest length as a parameter and the .c never
# reads DIGEST_BIT_LENGTH, so the same sources are built three times with the
# instance name / digest size set through the shim.
define MOZI_XOF
SHIMDEFS_$(1)-$(2) := -DNGCC_INSTANCE='"$(1)-$(2)"' -DNGCC_DIGEST_BITS=$(2)
$$(eval $$(call ngcc_instance,$(1)-$(2),$(REF)/$(1)))
endef
$(eval $(call MOZI_XOF,MOZI-384-XOF,256))
$(eval $(call MOZI_XOF,MOZI-384-XOF,1024))
$(eval $(call MOZI_XOF,MOZI-384-XOF,2048))
$(eval $(call MOZI_XOF,MOZI-512-XOF,256))
$(eval $(call MOZI_XOF,MOZI-512-XOF,1024))
$(eval $(call MOZI_XOF,MOZI-512-XOF,2048))

include ../api/link_finish.mk

.PHONY: reproduce-forum
reproduce-forum:
	python3 validate_spec_marker.py
