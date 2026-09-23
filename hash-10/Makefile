# NGCC uniform build: hash-10 FEILIAN (reference implementation)
NGCC_ID   := hash-10
NGCC_TYPE := hash
NGCC_ALG  := FEILIAN
include ../api/link_rules.mk

REF := FEILIAN/Implementations/Reference_Implementation
NGCC_KAT_DIR := FEILIAN/Test_Vectors

# Each instance dir holds CryptHash_AlgorithmInstance.c + drng.c (default glob).
# Optimized_Implementation (SIMD) and Additional_Implementation (SystemVerilog)
# are not built.
$(eval $(call ngcc_instance,FEILIAN512,$(REF)/FEILIAN512))
$(eval $(call ngcc_instance,FEILIAN768,$(REF)/FEILIAN768))
$(eval $(call ngcc_instance,FEILIAN1024,$(REF)/FEILIAN1024))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libFEILIAN512.so
	$(MAKE) -C ../security hash_oom_false_success
	../security/hash_oom_false_success lib/libFEILIAN512.so feilian hash-10-1
