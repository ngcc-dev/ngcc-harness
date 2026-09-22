# NGCC uniform build: kem-36 TRIKE (reference implementation)
NGCC_ID   := kem-36
NGCC_TYPE := kem
NGCC_ALG  := TRIKE
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# Per the CMakeLists.txt: all of ICCS/*.c (official drng.c/auxfunc.c, identical
# modulo whitespace/CRLF) and src/*.c, -I both; tests/KAT_KEM.c is the driver.
# The API implementation is src/KEM_AlgorithmInstance.c drawing from
# drng_algorithm.
TRIKE_SRCS := ICCS/auxfunc.c ICCS/drng.c src/KEM_AlgorithmInstance.c src/decoder.c src/gf2x.c src/sample.c

define TRIKE
SRCS_$(1) := $(TRIKE_SRCS)
INC_$(1)  := -Isrc/$(1)/ICCS -Isrc/$(1)/src
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call TRIKE,TRIKE-2))
$(eval $(call TRIKE,TRIKE-5))
$(eval $(call TRIKE,TRIKE-7))
$(eval $(call TRIKE,TRIKE-9))

include ../api/link_finish.mk
