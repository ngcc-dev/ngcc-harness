# NGCC uniform build: hash-19 MoFang Hash Function (reference implementation)
NGCC_ID   := hash-19
NGCC_TYPE := hash
NGCC_ALG  := MoFang
include ../api/link_rules.mk

REF := MoFang/Implementations/Reference_Implementation
NGCC_KAT_DIR := MoFang/Test_Vectors/Test_Vector

# Each instance dir holds CryptHash_AlgorithmInstance.c + drng.c (identical to
# api/drng.c); the default glob is sufficient. ALGORITHM_INSTANCE uses
# underscores (MoFang_512) and so do the KAT file names.
$(eval $(call ngcc_instance,MoFang-256,$(REF)/MoFang-256))
$(eval $(call ngcc_instance,MoFang-512,$(REF)/MoFang-512))
$(eval $(call ngcc_instance,MoFang-768,$(REF)/MoFang-768))
$(eval $(call ngcc_instance,MoFang-1024,$(REF)/MoFang-1024))
$(eval $(call ngcc_instance,MoFang-256-XOF,$(REF)/MoFang-256-XOF))
$(eval $(call ngcc_instance,MoFang-768-XOF,$(REF)/MoFang-768-XOF))

include ../api/link_finish.mk

.PHONY: reproduce
reproduce: all
	python3 reproduce.py
