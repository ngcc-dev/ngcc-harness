# NGCC uniform build: kem-38 UVW-KEM (reference implementation)
NGCC_ID   := kem-38
NGCC_TYPE := kem
NGCC_ALG  := UVW-KEM
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per the CMakeLists.txt: src/*.c minus the KAT_KEM.c driver, headers in
# include/. drng.c/auxfunc.c are the official files; KEM_AlgorithmInstance.c
# draws from drng_algorithm. ALGORITHM_INSTANCE is "UVW_KEM_<n>" (underscores)
# while the directory is UVW-KEM-<n>; KAT files follow the former.
UVW_SRCS := src/KEM_AlgorithmInstance.c src/drng.c src/auxfunc.c src/gf_math.c src/list_decoding.c src/uvw_constants.c

define UVW
SRCS_$(1) := $(UVW_SRCS)
INC_$(1)  := -Isrc/$(1)/include
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"include/KEM_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call UVW,UVW-KEM-128))
$(eval $(call UVW,UVW-KEM-256))
$(eval $(call UVW,UVW-KEM-512))

include ../api/link_finish.mk
