# NGCC uniform build: sign-10 Facto-DSA (reference implementation)
NGCC_ID   := sign-10
NGCC_TYPE := sig
NGCC_ALG  := Facto-DSA
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# Each instance dir holds SIG_AlgorithmInstance.c (parameters via FACTO_N/M/Q in
# the header), plus verbatim copies of the ICCS drng.c/auxfunc.c. The default
# glob drops KAT_SIG.c, test.c and run_benchmarks.c. Without -DFACTODSA_SO_BUILD
# the code uses `extern DRNG_ctx drng_algorithm` (our shim's); with that define
# it would define its own context seeded from getentropy(), so we do NOT set it.
# Only KAT files for -128 and -256 were submitted; -512 is built but has no KAT.
$(eval $(call ngcc_instance,Facto-DSA-128,$(REF)/Facto-DSA-128))
$(eval $(call ngcc_instance,Facto-DSA-256,$(REF)/Facto-DSA-256))
$(eval $(call ngcc_instance,Facto-DSA-512,$(REF)/Facto-DSA-512))

# The submission contains KAT files for 128 and 256 only. Keep 512 as a built
# library, but do not turn the absence of a submitted reference file into a
# test failure.
TESTS := $(filter-out test-Facto-DSA-512,$(TESTS))
NGCC_TEST_INSTANCES := Facto-DSA-128 Facto-DSA-256

include ../api/link_finish.mk

.PHONY: reproduce-forgery
reproduce-forgery: all
	python3 reproduce_forgery.py
