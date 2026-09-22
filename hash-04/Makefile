# NGCC uniform build: hash-04 CHAMP (reference implementation)
NGCC_ID   := hash-04
NGCC_TYPE := hash
NGCC_ALG  := CHAMP
include ../api/link_rules.mk

REF := CHAMP/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation
NGCC_KAT_DIR := CHAMP/Implementations and Test_Vectors/API_CryptHash/Test_Vectors

$(eval $(call ngcc_instance,CHAMP-512,$(REF)/CHAMP-512))
$(eval $(call ngcc_instance,CHAMP-1024,$(REF)/CHAMP-1024))

include ../api/link_finish.mk

.PHONY: reproduce reproduce-full
reproduce: all
	python3 reproduce.py --mitm-bits 32

reproduce-full: all
	python3 reproduce.py --mitm-bits 40
