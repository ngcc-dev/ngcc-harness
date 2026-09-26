# NGCC uniform build: sign-27 SQIsignTriangle (reference implementation)
NGCC_ID   := sign-27
NGCC_TYPE := sig
NGCC_ALG  := SQIsignTriangle
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Each SQIsignTriangle_lvlN dir wraps a copy of the SQIsign C project
# (sqisigntriangle/, SVARIANT_S=lvlN, SQISIGN_BUILD_TYPE=ref, GMP_LIBRARY=SYSTEM
# => -lgmp). Source list from sqisigntriangle/src/**/CMakeLists.txt and
# lvlx.cmake for the sqisign_lvlN_test_nistapi target that KAT_SIG links.
# Randomness: SIG_AlgorithmInstance.c itself defines randombytes() on top of
# drng_algorithm, so the library's randombytes_system.c/randombytes_ctrdrbg.c
# (and aes_c.c, only needed by the CTR-DRBG) are not compiled. The bundled
# common/generic/fips202.c is the candidate's SM3/pseudoXOF adapter over
# auxfunc.c (not Keccak). Defines from CMakeLists.txt/.cmake/target.cmake
# on x86_64 Linux: SQISIGN_BUILD_TYPE_REF SQISIGN_GF_IMPL_REF ENABLE_SIGN
# TARGET_AMD64 RADIX_64 HAVE_UINT128 TARGET_OS_UNIX SQISIGN_TEST_REPS=10.
S := sqisigntriangle/src
TRI_COMMON := SIG_AlgorithmInstance.c drng.c auxfunc.c \
    $(S)/sqisign.c \
    $(S)/common/generic/fips202.c $(S)/common/generic/mem.c $(S)/common/generic/tools.c \
    $(S)/quaternion/ref/generic/intbig.c $(S)/quaternion/ref/generic/algebra.c \
    $(S)/quaternion/ref/generic/ideal.c $(S)/quaternion/ref/generic/dim4.c \
    $(S)/quaternion/ref/generic/dim2.c $(S)/quaternion/ref/generic/integers.c \
    $(S)/quaternion/ref/generic/lattice.c $(S)/quaternion/ref/generic/lat_ball.c \
    $(S)/quaternion/ref/generic/finit.c $(S)/quaternion/ref/generic/printer.c \
    $(S)/quaternion/ref/generic/lll/l2.c $(S)/quaternion/ref/generic/lll/lll_verification.c \
    $(S)/quaternion/ref/generic/lll/lll_applications.c $(S)/quaternion/ref/generic/rationals.c \
    $(S)/quaternion/ref/generic/normeq.c $(S)/quaternion/ref/generic/qlapoti.c \
    $(S)/quaternion/ref/generic/hnf/ibz_division.c $(S)/quaternion/ref/generic/hnf/hnf_internal.c \
    $(S)/quaternion/ref/generic/hnf/hnf.c $(S)/quaternion/ref/generic/test/random_input_generation.c \
    $(S)/mp/ref/generic/mp.c \
    $(S)/gf/ref/lvlx/fp.c $(S)/gf/ref/lvlx/fp2.c \
    $(S)/ec/ref/lvlx/ec.c $(S)/ec/ref/lvlx/ec_jac.c $(S)/ec/ref/lvlx/xisog.c \
    $(S)/ec/ref/lvlx/xeval.c $(S)/ec/ref/lvlx/isog_chains.c $(S)/ec/ref/lvlx/basis.c \
    $(S)/ec/ref/lvlx/biextension.c \
    $(S)/hd/ref/lvlx/hd.c $(S)/hd/ref/lvlx/theta_structure.c $(S)/hd/ref/lvlx/theta_isogenies.c \
    $(S)/verification/ref/lvlx/common.c $(S)/verification/ref/lvlx/encode_verification.c \
    $(S)/verification/ref/lvlx/verify.c \
    $(S)/id2iso/ref/lvlx/id2iso.c $(S)/id2iso/ref/lvlx/dim2id2iso.c \
    $(S)/signature/ref/lvlx/encode_signature.c $(S)/signature/ref/lvlx/keygen.c \
    $(S)/signature/ref/lvlx/sign.c

# $(1)=label (= dir name) $(2)=level $(3)=gf prime file
define TRI
SRCS_$(1) := $(TRI_COMMON) $(S)/nistapi/$(2)/api.c $(S)/gf/ref/$(2)/$(3) \
    $(S)/precomp/ref/$(2)/ec_params.c $(S)/precomp/ref/$(2)/e0_basis.c \
    $(S)/precomp/ref/$(2)/hd_splitting_transforms.c $(S)/precomp/ref/$(2)/torsion_constants.c \
    $(S)/precomp/ref/$(2)/quaternion_data.c $(S)/precomp/ref/$(2)/endomorphism_action.c
INC_$(1) := -Isrc/$(1)/sqisigntriangle/include -Isrc/$(1)/$(S)/nistapi/$(2) \
    -Isrc/$(1)/$(S)/common/generic/include -Isrc/$(1)/$(S)/common/generic \
    -Isrc/$(1)/$(S)/quaternion/ref/generic/include \
    -Isrc/$(1)/$(S)/quaternion/ref/generic/internal_quaternion_headers \
    -Isrc/$(1)/$(S)/quaternion/ref/generic \
    -Isrc/$(1)/$(S)/mp/ref/generic/include -Isrc/$(1)/$(S)/gf/ref/include \
    -Isrc/$(1)/$(S)/precomp/ref/$(2)/include -Isrc/$(1)/$(S)/ec/ref/include \
    -Isrc/$(1)/$(S)/hd/ref/include -Isrc/$(1)/$(S)/hd/ref/lvlx \
    -Isrc/$(1)/$(S)/id2iso/ref/include -Isrc/$(1)/$(S)/verification/ref/include \
    -Isrc/$(1)/$(S)/signature/ref/include
DEFS_$(1) := -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DENABLE_SIGN -DTARGET_AMD64 \
    -DRADIX_64 -DHAVE_UINT128 -DTARGET_OS_UNIX -DSQISIGN_TEST_REPS=10 -DSQISIGN_VARIANT=$(2)
CFLAGS_$(1) := -funroll-loops
LDLIBS_$(1) := -lgmp
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call TRI,SQIsignTriangle_lvl1,lvl1,fp_p5248_64.c))
$(eval $(call TRI,SQIsignTriangle_lvl2,lvl2,fp_p9309_64.c))
$(eval $(call TRI,SQIsignTriangle_lvl5,lvl5,fp_p27500_64.c))
$(eval $(call TRI,SQIsignTriangle_lvl6,lvl6,fp_p1131016_64.c))

include ../api/link_finish.mk

.PHONY: exploit-modular-challenge
exploit-modular-challenge:
	python3 reproduce_modular_challenge.py
