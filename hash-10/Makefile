# NGCC uniform build: hash-10 FEILIAN (reference implementation)
NGCC_ID   := hash-10
NGCC_TYPE := hash
NGCC_ALG  := FEILIAN
include ../api/link_rules.mk

REF := FEILIAN/Implementations/Reference_Implementation
NGCC_KAT_DIR := FEILIAN/Test_Vectors

# Each instance dir holds CryptHash_AlgorithmInstance.c + drng.c (default glob).
# The normal library build uses reference C. The RTL witness below simulates
# the submitted Additional_Implementation (SystemVerilog) separately.
$(eval $(call ngcc_instance,FEILIAN512,$(REF)/FEILIAN512))
$(eval $(call ngcc_instance,FEILIAN768,$(REF)/FEILIAN768))
$(eval $(call ngcc_instance,FEILIAN1024,$(REF)/FEILIAN1024))

include ../api/link_finish.mk

.PHONY: exploit rtl-exploit unused-bits reproduce-new
exploit: lib/libFEILIAN512.so
	$(MAKE) -C ../security hash_oom_false_success
	../security/hash_oom_false_success lib/libFEILIAN512.so feilian hash-10-1

rtl-exploit:
	python3 reproduce_rtl_length_collision.py

unused-bits: libs
	python3 reproduce_unused_bits.py

reproduce-new: rtl-exploit unused-bits
