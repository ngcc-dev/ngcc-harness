# NGCC uniform build: hash-02 AXIS (reference implementation)
NGCC_ID   := hash-02
NGCC_TYPE := hash
NGCC_ALG  := AXIS
include ../api/link_rules.mk

REF := AXIS/Implementations/Reference_Implementation
NGCC_KAT_DIR := AXIS/Test_Vectors

# Per the candidate Makefile: CryptHash_AlgorithmInstance.c axis_core.c drng.c
# (default glob). The shipped drng.c only adds comments over api/drng.c.
$(eval $(call ngcc_instance,AXIS-512,$(REF)/AXIS-512))
$(eval $(call ngcc_instance,AXIS-768,$(REF)/AXIS-768))
$(eval $(call ngcc_instance,AXIS-1024,$(REF)/AXIS-1024))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libAXIS-512.so
	$(MAKE) -C ../security hash_oom_false_success
	../security/hash_oom_false_success lib/libAXIS-512.so axis hash-02-1

.PHONY: reproduce-inverse
reproduce-inverse: build/axis_inverse
	./build/axis_inverse

build/axis_inverse: reproduce_invertible_step.c
	@mkdir -p build
	$(CC) -O2 -std=gnu11 -I$(REF)/AXIS-1024 -o $@ $<
