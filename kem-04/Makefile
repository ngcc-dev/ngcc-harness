# NGCC uniform build: kem-04 BAG-Piglet (reference implementation)
NGCC_ID   := kem-04
NGCC_TYPE := kem
NGCC_ALG  := BAG-Piglet
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per CMakeLists.txt (target <instance>-kat): src/{common,scheme,ffi,rbc}/*.c,
# lib/api_pkc/{drng,auxfunc}.c (auxfunc.c differs from api/ only in CRLF),
# lib/randombytes/randombytes.c and src/kat/KEM_AlgorithmInstance.c, compiled
# with -DBAG_PIGLET_KAT=1. That define makes CryptoRandomBytes() (used for the
# keygen seed and the encaps salt) draw from drng_algorithm instead of
# getrandom()/dev/urandom. drng_algorithm is defined in the KAT driver, so the
# shim provides it. CMake also adds -mpclmul when available; the sources build
# without it. pthread is linked by CMake but never used.
PIGLET_SRCS := src/common/aes256ctr.c src/common/augabidulin.c src/common/ccakem.c \
               src/common/ccapke.c src/common/gabidulin.c src/common/parsing.c \
               src/common/q_polynomial.c src/common/tools.c \
               src/ffi/ffi_elt.c src/ffi/ffi_field.c src/ffi/ffi_vec.c \
               src/rbc/rbc_elt.c src/rbc/rbc_poly.c src/rbc/rbc_qre.c src/rbc/rbc_vec.c \
               src/rbc/rbc_vspace.c src/scheme/bag_piglet.c \
               src/kat/KEM_AlgorithmInstance.c \
               lib/api_pkc/auxfunc.c lib/api_pkc/drng.c lib/randombytes/randombytes.c

# label, dir
define PIGLET
SRCS_$(1) := $(PIGLET_SRCS)
INC_$(1)  := -Isrc/$(1)/src/common -Isrc/$(1)/src/scheme -Isrc/$(1)/src/ffi -Isrc/$(1)/src/rbc \
             -Isrc/$(1)/src/kat -Isrc/$(1)/lib/api_pkc -Isrc/$(1)/lib/randombytes
DEFS_$(1) := -DBAG_PIGLET_KAT=1
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"src/kat/KEM_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(2)))
endef

$(eval $(call PIGLET,bag_piglet_128,bag_piglet128))
$(eval $(call PIGLET,bag_piglet_256,bag_piglet256))
$(eval $(call PIGLET,bag_piglet_384,bag_piglet384))
$(eval $(call PIGLET,bag_piglet_512,bag_piglet512))

include ../api/link_finish.mk
