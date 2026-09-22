# NGCC uniform build: hash-21 Neulaser (reference implementation)
NGCC_ID   := hash-21
NGCC_TYPE := hash
NGCC_ALG  := Neulaser
include ../api/link_rules.mk

REF := Neulaser/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation
NGCC_KAT_DIR := Neulaser/Implementations and Test_Vectors/API_CryptHash/Test_Vector

# Per the candidate Makefile: CryptHash_AlgorithmInstance.c + drng.c (identical
# to api/drng.c). The shipped KAT_CryptHash.exe binaries are ignored; the
# Others/ self-assessment tree is not used.
$(eval $(call ngcc_instance,Neulaser-512,$(REF)/Neulaser-512))
$(eval $(call ngcc_instance,Neulaser-768,$(REF)/Neulaser-768))
$(eval $(call ngcc_instance,Neulaser-1024,$(REF)/Neulaser-1024))

include ../api/link_finish.mk

.PHONY: reproduce
reproduce: all
	python3 reproduce.py
