# NGCC uniform build: kem-31 QIMEN-PIKE (reference implementation)
NGCC_ID   := kem-31
NGCC_TYPE := kem
NGCC_ALG  := QIMEN-PIKE
include ../api/link_rules.mk

# Single CMake source tree shared by all three instances (SQIsign-style
# layout). The NGCC adapter lives in src/ngcc/common and selects the parameter
# set through -DNGCC_PIKE_LEVEL=n and the per-level precomp/ and gf/ dirs
# (target ngcc_pike_kat_<instance> in src/ngcc/CMakeLists.txt). Only the
# "ref" implementation type and the compressed PIKE variant are used, as in the
# candidate's KAT target. Top-level Test_Vectors is identical to
# Implementations/Test_Vector (and the *-Optimized KATs are byte-identical to
# the reference ones).
REF := Implementations/Implementations
NGCC_KAT_DIR := Test_Vectors

# Source list = union of the CMake libraries linked by the KAT target:
# pike_pike_<L>_compressed, pike_id2iso_<L>, pike_hd_<L>, pike_intbig_generic,
# pike_common_sys, pike_ec_<L>, pike_precomp_<L>, pike_gf_<L>, pike_klpt_<L>,
# pike_quaternion_generic. From pike_common_sys we drop randombytes_system.c
# (getrandom / /dev/urandom): KEM_AlgorithmInstance.c defines randombytes()
# itself on top of drng_algorithm, which is what the KAT build ends up using
# (its object definition pre-empts the archive member). drng.c/auxfunc.c in
# src/ngcc/common are the official ones (auxfunc.c differs from api/ only by
# line endings). fp_p<N>.c #includes modarith_p<N>.c.
# GMP: intbig/quaternion/klpt use libgmp (system gmp.h at
# /usr/include/x86_64-linux-gnu, on gcc's default path) -> -lgmp.
# Defines mirror .cmake/target.cmake (x86_64 Linux) and the top-level
# CMakeLists default PIKE_NGCC_XOF_BACKEND=sm3 (PIKE_XOF_BACKEND=2), which is
# what the shipped KATs were generated with.
PIKE_COMMON := src/ngcc/common/KEM_AlgorithmInstance.c src/ngcc/common/drng.c src/ngcc/common/auxfunc.c \
    src/common/generic/aes_c.c src/common/generic/fips202.c src/common/generic/pike_hash.c src/common/generic/tools.c \
    src/intbig/ref/generic/intbig.c \
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c src/quaternion/ref/generic/dim4.c \
    src/quaternion/ref/generic/dim2.c src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c \
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c src/quaternion/ref/generic/lll.c \
    src/quaternion/ref/generic/matkermod.c \
    src/klpt/ref/klptx/tools.c \
    src/gf/ref/gfx/fp2.c src/gf/ref/gfx/mp.c \
    src/ec/ref/pike_ecx/ec.c src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c \
    src/ec/ref/ecx/isog_chains.c src/ec/ref/pike_ecx/basis.c src/ec/ref/ecx/biextension.c \
    src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c src/hd/ref/hdx/theta_isogenies.c \
    src/id2iso/ref/id2isox/id2iso.c \
    src/pike/ref/pikex_compressed/pike_compressed.c

# label, level dir (NGCC_n), level id, gf prime file, extra defines
define PIKE
SRCS_$(1) := $(PIKE_COMMON) src/gf/ref/$(2)/$(4) \
    src/precomp/ref/$(2)/gf_constants.c src/precomp/ref/$(2)/torsion_constants.c \
    src/precomp/ref/$(2)/endomorphism_action.c src/precomp/ref/$(2)/klpt_constants.c \
    src/precomp/ref/$(2)/quaternion_data.c
INC_$(1) := -Isrc/$(1)/src/ngcc/common -Isrc/$(1)/include \
    -Isrc/$(1)/src/common/generic/include -Isrc/$(1)/src/intbig/ref/generic/include \
    -Isrc/$(1)/src/quaternion/ref/generic/include -Isrc/$(1)/src/precomp/ref/$(2)/include \
    -Isrc/$(1)/src/klpt/ref/include -Isrc/$(1)/src/gf/ref/include -Isrc/$(1)/src/ec/ref/include \
    -Isrc/$(1)/src/hd/ref/include -Isrc/$(1)/src/id2iso/ref/include \
    -Isrc/$(1)/src/pike/ref/include -Isrc/$(1)/src/pike/ref/pikex_compressed/include
DEFS_$(1) := -DALGORITHM_INSTANCE='"$(1)"' -DNGCC_PIKE_LEVEL=$(3) -DOUTPUT_BLANK_TEST_VECTORS=0 \
    -DPIKE_XOF_BACKEND=2 -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX $(5)
LDLIBS_$(1) := -lgmp
$$(eval $$(call ngcc_instance,$(1),$(REF)))
endef

$(eval $(call PIKE,NGCC-1,NGCC_1,1,fp_p479.c,))
$(eval $(call PIKE,NGCC-2,NGCC_2,2,fp_p765.c,))
$(eval $(call PIKE,NGCC-3,NGCC_3,3,fp_p1534.c,-DPIKE_COMPRESSED_SHARED_SECRET_BYTES=64))

include ../api/link_finish.mk
