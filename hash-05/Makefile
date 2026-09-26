# NGCC uniform build: hash-05 uHash (reference implementation)
NGCC_ID   := hash-05
NGCC_TYPE := hash
NGCC_ALG  := uHash
include ../api/link_rules.mk

# Non-standard layout: the ICCS tree is nested one level deeper under
# numbered Chinese-named folders.
REF := uHash/Implementations/3 实现代码/Implementations/Reference_Implementation
NGCC_KAT_DIR := uHash/Test_Vectors/4 测试向量/Test_Vectors

# No candidate build file; the whole hash lives in CryptHash_AlgorithmInstance.c
# plus the ICCS drng.c (default glob).
$(eval $(call ngcc_instance,uHash-512,$(REF)/uHash-512))
$(eval $(call ngcc_instance,uHash-768,$(REF)/uHash-768))
$(eval $(call ngcc_instance,uHash-1024,$(REF)/uHash-1024))

include ../api/link_finish.mk

.PHONY: exploit reproduce
exploit reproduce: libs
	python3 reproduce_partial_bit_collision.py
