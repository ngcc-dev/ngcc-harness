# NGCC uniform build: hash-14 Laurus (reference implementation)
NGCC_ID   := hash-14
NGCC_TYPE := hash
NGCC_ALG  := Laurus
include ../api/link_rules.mk

REF := Laurus/Implementations/Reference_Implementation
NGCC_KAT_DIR := Laurus/Test_Vectors

# Per README: gcc CryptHash_AlgorithmInstance.c drng.c KAT_CryptHash.c laurus_NNN.c
# (default glob minus the KAT driver). ALGORITHM_INSTANCE is lower-case
# (laurus_512 ...) and matches the KAT file names; labels follow the dir names.
$(eval $(call ngcc_instance,Laurus-512,$(REF)/Laurus-512))
$(eval $(call ngcc_instance,Laurus-768,$(REF)/Laurus-768))
$(eval $(call ngcc_instance,Laurus-1024,$(REF)/Laurus-1024))
$(eval $(call ngcc_instance,Laurus-XOF,$(REF)/Laurus-XOF))

include ../api/link_finish.mk

.PHONY: reproduce
reproduce: all build/domain-separation-repro
	./build/domain-separation-repro

build/domain-separation-repro: reproduce.c $(REF)/Laurus-1024/laurus_1024.c | build
	$(CC) $(CFLAGS) -Dstatic= -I$(REF)/Laurus-1024 $^ -o $@
