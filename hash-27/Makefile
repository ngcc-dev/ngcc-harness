# NGCC uniform build: hash-27 The Vedak Hash Function Family (reference implementation)
NGCC_ID   := hash-27
NGCC_TYPE := hash
NGCC_ALG  := Vedak
include ../api/link_rules.mk

REF := Vedak/Implementations/Reference_Implementation
NGCC_KAT_DIR := Vedak/Test_Vectors

# CryptHash_AlgorithmInstance.c + drng.c (identical to api/drng.c); default glob.
$(eval $(call ngcc_instance,Vedak-512,$(REF)/Vedak-512))
$(eval $(call ngcc_instance,Vedak-768,$(REF)/Vedak-768))
$(eval $(call ngcc_instance,Vedak-1024,$(REF)/Vedak-1024))

include ../api/link_finish.mk
