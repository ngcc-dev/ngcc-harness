# NGCC uniform build: hash-26 The Hash Function CHIME (reference implementation)
NGCC_ID   := hash-26
NGCC_TYPE := hash
NGCC_ALG  := CHIME
include ../api/link_rules.mk

REF := CHIME/Implementations/Reference_Implementation
NGCC_KAT_DIR := CHIME/Test_Vectors/Test_Vector

# Per the candidate makefile: KAT_CryptHash.c CryptHash_AlgorithmInstance.c drng.c,
# LDLIBS empty. Only the Optimized_Implementation tree pulls in OpenSSL
# (sha_openssl.h, -lcrypto); the reference code is self-contained, so no
# -lcrypto here. drng.c is identical to api/drng.c. Default glob.
$(eval $(call ngcc_instance,CHIME-512,$(REF)/CHIME-512))
$(eval $(call ngcc_instance,CHIME-1024,$(REF)/CHIME-1024))

include ../api/link_finish.mk

.PHONY: reproduce
reproduce: all
	python3 reproduce.py
