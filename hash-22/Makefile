# NGCC uniform build: hash-22 Pavelor (reference implementation)
NGCC_ID   := hash-22
NGCC_TYPE := hash
NGCC_ALG  := Pavelor
include ../api/link_rules.mk

REF := Pavelor/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation
NGCC_KAT_DIR := Pavelor/Implementations and Test_Vectors/API_CryptHash/Test_Vector

# CryptHash_AlgorithmInstance.c + drng.c (identical to api/drng.c); default glob.
$(eval $(call ngcc_instance,Pavelor-512,$(REF)/Pavelor-512))
$(eval $(call ngcc_instance,Pavelor-768,$(REF)/Pavelor-768))
$(eval $(call ngcc_instance,Pavelor-1024,$(REF)/Pavelor-1024))

include ../api/link_finish.mk
