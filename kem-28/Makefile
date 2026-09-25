# NGCC uniform build: kem-28 OAEP-NTRU (reference implementation)
NGCC_ID   := kem-28
NGCC_TYPE := kem
NGCC_ALG  := OAEP-NTRU
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the candidate Makefile: auxfunc.c drng.c poly.c ntt.c symmetric.c
# KEM_AlgorithmInstance.c (drng/auxfunc identical to api/). The default glob
# picks exactly these; KAT_KEM.c is the driver. KEM_AlgorithmInstance.c draws
# from drng_algorithm. Additional_Implementation is not built.
$(eval $(call ngcc_instance,OAEP-NTRU-648,$(REF)/OAEP-NTRU-648))
$(eval $(call ngcc_instance,OAEP-NTRU-1296,$(REF)/OAEP-NTRU-1296))
$(eval $(call ngcc_instance,OAEP-NTRU-2592,$(REF)/OAEP-NTRU-2592))

include ../api/link_finish.mk
