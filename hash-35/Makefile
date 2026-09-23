# NGCC uniform build: hash-35 Wish (reference implementation)
NGCC_ID   := hash-35
NGCC_TYPE := hash
NGCC_ALG  := Wish
include ../api/link_rules.mk

REF := Wish/Implementations/Reference_Implementation

# Per each instance's Makefile: CryptHash_AlgorithmInstance.c wish.c plus the
# unmodified official drng.c (KAT_CryptHash.c is the driver). Labels follow
# ALGORITHM_INSTANCE ("Wish512"/"Wish1024"); the KAT files live in one
# subdirectory per instance under Test_Vectors/Test_Vector/. The Optimized_/
# Additional_Implementation trees and Others/ are ignored.
define WISH
KATDIR_$(1) := Wish/Test_Vectors/Test_Vector/$(1)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)_reference))
endef

$(eval $(call WISH,Wish512))
$(eval $(call WISH,Wish1024))

include ../api/link_finish.mk
