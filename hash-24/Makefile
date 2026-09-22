# NGCC uniform build: hash-24 QuantaSylva Hash / QSH (reference implementation)
NGCC_ID   := hash-24
NGCC_TYPE := hash
NGCC_ALG  := QSH
include ../api/link_rules.mk

# Non-standard layout: Implementations/03_Implementations/1_Reference_Implementation
# (3_Additional_Implementation is not used) and Test_Vectors/04_TestVectors.
REF := QSH/Implementations/03_Implementations/1_Reference_Implementation
NGCC_KAT_DIR := QSH/Test_Vectors/04_TestVectors

# Per CMakeLists.txt / build.sh: drng.c KAT_CryptHash.c CryptHash_AlgorithmInstance.c
# (-std=c99); drng.c is identical to api/drng.c. Default glob; neither the
# CMake project nor build.sh is executed.
$(eval $(call ngcc_instance,QSH-512,$(REF)/QSH-512))
$(eval $(call ngcc_instance,QSH-768,$(REF)/QSH-768))
$(eval $(call ngcc_instance,QSH-1024,$(REF)/QSH-1024))

include ../api/link_finish.mk

.PHONY: reproduce
reproduce: reproduce_invariant
	./reproduce_invariant

reproduce_invariant: reproduce_invariant.c
	$(CC) -O2 -std=c99 -I. $< -o $@
