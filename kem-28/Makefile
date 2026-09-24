# NGCC uniform build: kem-28 OAEP-NTRU (reference implementation)
NGCC_ID   := kem-28
NGCC_TYPE := kem
NGCC_ALG  := OAEP-NTRU
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the candidate Makefiles: auxfunc.c drng.c poly.c ntt.c symmetric.c and
# KEM_AlgorithmInstance.c. The default source selection picks exactly these;
# KAT_KEM.c is the driver. Optimized and additional implementations are not built.
$(eval $(call ngcc_instance,OAEP-NTRU-648,$(REF)/OAEP-NTRU-648))
$(eval $(call ngcc_instance,OAEP-NTRU-1296,$(REF)/OAEP-NTRU-1296))
$(eval $(call ngcc_instance,OAEP-NTRU-2592,$(REF)/OAEP-NTRU-2592))

include ../api/link_finish.mk
