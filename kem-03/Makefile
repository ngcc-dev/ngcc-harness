# NGCC uniform build: kem-03 BAG-Loong (reference implementation)
NGCC_ID   := kem-03
NGCC_TYPE := kem
NGCC_ALG  := BAG-Loong
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from each instance's Makefile (SRCS := ...) minus test/KAT_KEM.c:
# src/*.c, the rank-metric library lib/rbc-<m>/ (m differs per instance) and
# the official lib/api_pkc/{drng,auxfunc}.c (byte-identical to api/).
# src/loong_api_random.c draws from drng_algorithm; no OS entropy anywhere.
LOONG_SRCS := src/KEM_AlgorithmInstance.c src/augabidulin.c src/ct_util.c src/gabidulin.c \
              src/loong_api_random.c src/loong_hash.c src/loong_kem.c src/loong_pke.c \
              src/loong_support.c src/loong_xof_reader.c src/parsing.c src/qpoly.c \
              lib/api_pkc/auxfunc.c lib/api_pkc/drng.c

# label, dir, rbc-m
define LOONG
SRCS_$(1) := $(LOONG_SRCS) lib/rbc-$(3)/rbc_elt.c lib/rbc-$(3)/rbc_vec.c
INC_$(1)  := -Isrc/$(1)/src -Isrc/$(1)/lib/api_pkc -Isrc/$(1)/lib/rbc-$(3)
CFLAGS_$(1) := -std=c99
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(2)))
endef

$(eval $(call LOONG,BAG-Loong-128,Loong-Block-ms-128,47))
$(eval $(call LOONG,BAG-Loong-256,Loong-Block-ms-256,67))
$(eval $(call LOONG,BAG-Loong-384,Loong-Block-ms-384,83))
$(eval $(call LOONG,BAG-Loong-512,Loong-Block-ms-512,97))

include ../api/link_finish.mk
