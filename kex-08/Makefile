# NGCC uniform build: kex-08 NIIKE (reference implementation)
NGCC_ID   := kex-08
NGCC_TYPE := kex
NGCC_ALG  := NIIKE
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# CMake layout: level-independent sources at the tree root (common/, gf/, ec/,
# protocols/, ngccapi/) plus per-level sources and headers under NIIKE-lvN/
# (precomp/, gf/fp.c fp2.c, ec/bundle_dac.c, protocols/bundleprotocols_internal.c,
# ngccapi/include/KEX_AlgorithmInstance.h). The source dir of every instance is
# the tree root; -DRADIX_64 as in the top-level CMakeLists.txt (64-bit only).
# common/drng.c is the official file; there is no auxfunc.c in the tree, so
# api/auxfunc.c is added by the rules. Randomness is drawn from drng_algorithm
# in ngccapi/KEX_AlgorithmInstance.c. Additional_Implementation is ignored.
NIIKE_SRCS := common/drng.c gf/bundle_fp2.c ec/bundle_ec.c ec/bundle_kps.c ec/bundle_xisog.c \
              ec/bundle_xeval.c ec/bundle_poly-mul.c ec/bundle_poly-redc.c ec/ec.c \
              protocols/bundleprotocols.c ngccapi/KEX_AlgorithmInstance.c

define NIIKE
SRCS_$(1) := $(NIIKE_SRCS) $(1)/precomp/torsion_constants.c $(1)/precomp/protocol_constants.c \
             $(1)/precomp/bundleprotocol_constants.c $(1)/gf/fp.c $(1)/gf/fp2.c $(1)/ec/bundle_dac.c \
             $(1)/protocols/bundleprotocols_internal.c
INC_$(1)  := -Isrc/$(1)/$(1)/ngccapi/include -Isrc/$(1)/common/include -Isrc/$(1)/gf/include \
             -Isrc/$(1)/$(1)/precomp/include -Isrc/$(1)/$(1)/ec/include -Isrc/$(1)/ec/include \
             -Isrc/$(1)/$(1)/protocols/include -Isrc/$(1)/protocols/include
DEFS_$(1) := -DRADIX_64
# Reference isogeny code is very slow (spec Table 7.2, gigacycles on i7-11700F:
# keygen/agree lv128 18/15, lv256 429/376, lv512 68323/67946). Measured here:
# lv128 ~26 s per KAT record, lv256 ~10 min per record, lv512 ~1 day per record.
TESTFLAGS_$(1) := --timeout 14400 $$(COUNT_$(1))
$$(eval $$(call ngcc_instance,$(1),$(REF)))
endef

# lv256: 3 of the 10 submitted records (~30 min); lv512: 1 record (~1 day)
COUNT_NIIKE-lv256 := --count 3
COUNT_NIIKE-lv512 := --count 1
$(eval $(call NIIKE,NIIKE-lv128))
$(eval $(call NIIKE,NIIKE-lv256))
# lv512 would need ~12 days for the 10-record KAT; build/run it only on request:
#   make NGCC_NIIKE_LV512=1 test-NIIKE-lv512
ifeq ($(NGCC_NIIKE_LV512),1)
$(eval $(call NIIKE,NIIKE-lv512))
endif

include ../api/link_finish.mk

PYTHON ?= sage -python
.PHONY: exploit
exploit: lib/libNIIKE-lv128.so
	$(PYTHON) reproduce_raw_key_distinguisher.py
