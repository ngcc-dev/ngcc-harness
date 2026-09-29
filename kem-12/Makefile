# NGCC uniform build: kem-12 CTL (reference implementation)
NGCC_ID   := kem-12
NGCC_TYPE := kem
NGCC_ALG  := CTL
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/KEM-CTL-x86-Reference_Implementation/crypto_kem/Implementations
NGCC_KAT_DIR := Implementations and Test_Vectors/KEM-CTL-x86-Reference_Implementation/crypto_kem/Test_Vectors

# Per each directory's Makefile (KAT_KEM target, CORE_OBJ + KEM_AlgorithmInstance.o):
# api_<q>_<n>.c (#includes api.c) codec.c fft.c fnr.c kem257.c kem769.c
# kem3329.c ntru_utils.c ng_{fxp,mp31,ntru,poly,zint31}.c sha3.c modqp.c prng.c
# auxfunc.c drng.c ntru_solver.c keygen.c KEM_AlgorithmInstance.c.
# The candidate passes -DNTRUGEN_PREFIX=ctlng to the ng_*.c/sha3.c objects
# only; it is given to every file here, which is equivalent: only ng_inner.h
# reads the macro and ntru_utils.c already #defines the same value.
# The candidate's -w is dropped. The shipped .o files and binaries are ignored.
# Key generation (ntru_solver.c) needs GMP and libquadmath -> LDLIBS_.
# prng.c (ctl_get_seed) draws from drng_algorithm.
# AArch64 has no libquadmath; there compat/aarch64/quadmath.h maps __float128
# onto long double, which is the same IEEE binary128 format (KATs pass).
ifeq ($(shell uname -m),aarch64)
CTL_INC  := -Icompat/aarch64
CTL_QUAD :=
else
CTL_INC  :=
CTL_QUAD := -lquadmath
endif
CTL_COMMON := codec.c fft.c fnr.c kem257.c kem769.c kem3329.c ntru_utils.c \
              ng_fxp.c ng_mp31.c ng_ntru.c ng_poly.c ng_zint31.c sha3.c modqp.c prng.c \
              auxfunc.c drng.c ntru_solver.c keygen.c KEM_AlgorithmInstance.c

# label (= ALGORITHM_INSTANCE = dir), api source
define CTL
SRCS_$(1)   := $(2) $(CTL_COMMON)
DEFS_$(1)   := -DNTRUGEN_PREFIX=ctlng
INC_$(1)    := $(CTL_INC)
LDLIBS_$(1) := $(CTL_QUAD) -lgmp
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call CTL,CTL-257-512,api_257_512.c))
$(eval $(call CTL,CTL-769-1024,api_769_1024.c))
$(eval $(call CTL,CTL-3329-2048,api_3329_2048.c))

include ../api/link_finish.mk
