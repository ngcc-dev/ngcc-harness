# NGCC uniform build: sign-04 CEDRUS-alpha (reference implementation)
NGCC_ID   := sign-04
NGCC_TYPE := sig
NGCC_ALG  := CEDRUS-alpha
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Same layout as sign-03 (CEDRUS+C): SPHINCS+-style, one directory per
# parameter set, candidate Makefile compiles every .c except KAT_SIG.c
# (= default glob). params.h hard-includes params/params-cedrusa-sm3-<set>.h,
# so -DPARAMS is not needed. randombytes.c is already the drng_algorithm
# wrapper; drng.c/auxfunc.c are the unmodified official copies.
$(eval $(call ngcc_instance,CEDRUSALPHA-160f,$(REF)/CEDRUSALPHA-160f))
$(eval $(call ngcc_instance,CEDRUSALPHA-160s,$(REF)/CEDRUSALPHA-160s))
$(eval $(call ngcc_instance,CEDRUSALPHA-256f,$(REF)/CEDRUSALPHA-256f))
$(eval $(call ngcc_instance,CEDRUSALPHA-256s,$(REF)/CEDRUSALPHA-256s))
$(eval $(call ngcc_instance,CEDRUSALPHA-384f,$(REF)/CEDRUSALPHA-384f))
$(eval $(call ngcc_instance,CEDRUSALPHA-384s,$(REF)/CEDRUSALPHA-384s))
$(eval $(call ngcc_instance,CEDRUSALPHA-512f,$(REF)/CEDRUSALPHA-512f))
$(eval $(call ngcc_instance,CEDRUSALPHA-512s,$(REF)/CEDRUSALPHA-512s))

include ../api/link_finish.mk

CEDRUS160S := $(REF)/CEDRUSALPHA-160s

.PHONY: exploit clean-exploit
exploit: reproduce_cedrus_alpha
	./reproduce_cedrus_alpha

reproduce_cedrus_alpha: reproduce_cedrus_alpha.c \
	$(CEDRUS160S)/wots.c $(CEDRUS160S)/address.c \
	$(CEDRUS160S)/hash_sm3.c $(CEDRUS160S)/auxfunc.c $(CEDRUS160S)/utils.c
	$(CC) -O2 -std=c99 -I$(CEDRUS160S) $^ -o $@

clean-exploit:
	rm -f reproduce_cedrus_alpha
