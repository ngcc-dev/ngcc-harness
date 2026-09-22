# NGCC uniform build: sign-09 DOVE (reference implementation)
NGCC_ID   := sign-09
NGCC_TYPE := sig
NGCC_ALG  := DOVE
include ../api/link_rules.mk

# Layout anomaly: the submission was shipped as .rar archives; the reference
# code lives under a self-assessment/benchmark harness tree. The actual
# API_PKC instances are the two directories below, each of which builds the
# three security levels via -DDOVE_PARAM_SET=DOVE{128,256,512} (see the
# candidate's Makefile / API_PKC/Implementations/Reference_Implementation/
# CMakeLists.txt). The shipped .o files and dove_sign_kat binaries are ignored.
REF := Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Source list from the candidate Makefile (OBJS minus KAT_SIG.o); drng.c and
# auxfunc.c are byte-identical to api/. SIG_AlgorithmInstance.h hard-codes
# ALGORITHM_INSTANCE "dove" for every instance, so the KAT-file name is given
# via -DNGCC_INSTANCE.
DOVE_SRCS := gf256.c matrix.c uov/blas_matrix_ref.c auxfunc.c drng.c SIG_AlgorithmInstance.c

# label, dir, param set
define DOVE
SRCS_$(1) := $(DOVE_SRCS)
INC_$(1)  := -Isrc/$(1)/uov
DEFS_$(1) := -DDOVE_PARAM_SET=DOVE$(3)
SHIMDEFS_$(1) := -DNGCC_INSTANCE='"$(1)"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(2)))
endef

$(eval $(call DOVE,dove_classic_128,DOVE_classic_ref,128))
$(eval $(call DOVE,dove_classic_256,DOVE_classic_ref,256))
$(eval $(call DOVE,dove_classic_512,DOVE_classic_ref,512))
$(eval $(call DOVE,dove_pkc_skc_128,DOVE_pkc_skc_ref,128))
$(eval $(call DOVE,dove_pkc_skc_256,DOVE_pkc_skc_ref,256))
$(eval $(call DOVE,dove_pkc_skc_512,DOVE_pkc_skc_ref,512))

include ../api/link_finish.mk

.PHONY: exploit
exploit:
	python3 validate_sm3_target.py
