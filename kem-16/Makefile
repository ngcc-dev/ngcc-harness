# NGCC uniform build: kem-16 HARE (reference implementation)
NGCC_ID   := kem-16
NGCC_TYPE := kem
NGCC_ALG  := HARE
include ../api/link_rules.mk

IMPL := Implementations and Test_Vectors/Implementations
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vectors

# Per CMakeLists.txt + cmake/HareAddInstance.cmake (ROLE Reference, CORE_KIND
# REF, BACKEND KR): the instance directory Reference_Implementation/HARE-<n>/kr
# holds only parameters.h/api.h/reed_solomon.h and KEM_AlgorithmInstance.c;
# the code is the shared _shared/hare_core/common/*.c + ref/*.c, compiled with
# -DHARE_SYMMETRIC_MODE_B -DHARE_BACKEND_KR=1 and the include order instance
# dir, common, ref, x86_64/avx256, api_pkc. The -std=c99 -Wpedantic -O2 of the
# candidate's reference build are replaced by the uniform flags.
# _shared/api_pkc/{drng,auxfunc}.c are lightly modified (UB-free rotl32) but
# output-equivalent copies of the official files; the official api/ copies are
# linked instead (harness default). symmetric_api_pkc.c draws from
# drng_algorithm. Shared-source paths are relative to src/<label> =
# Reference_Implementation/HARE-<n>/kr (the symlink resolves physically).
SH := ../../../_shared/hare_core
HARE_SRCS := KEM_AlgorithmInstance.c \
             $(SH)/common/code.c $(SH)/common/crypto_memset.c $(SH)/common/fft.c \
             $(SH)/common/kem.c $(SH)/common/parameter_contract.c $(SH)/common/symmetric_api_pkc.c \
             $(SH)/ref/compression_kr.c $(SH)/ref/gf2x.c $(SH)/ref/gf.c $(SH)/ref/hqc.c \
             $(SH)/ref/parsing.c $(SH)/ref/reed_muller.c $(SH)/ref/reed_solomon.c $(SH)/ref/vector.c

# label (= ALGORITHM_INSTANCE), level dir
define HARE
SRCS_$(1) := $(HARE_SRCS)
INC_$(1)  := -Isrc/$(1)/$(SH)/common -Isrc/$(1)/$(SH)/ref -Isrc/$(1)/$(SH)/x86_64/avx256 \
             -Isrc/$(1)/../../../_shared/api_pkc
DEFS_$(1) := -DHARE_SYMMETRIC_MODE_B -DHARE_BACKEND_KR=1
$$(eval $$(call ngcc_instance,$(1),$(IMPL)/Reference_Implementation/$(2)/kr))
endef

$(eval $(call HARE,HARE-128-kr,HARE-128))
$(eval $(call HARE,HARE-256-kr,HARE-256))
$(eval $(call HARE,HARE-384-kr,HARE-384))
$(eval $(call HARE,HARE-512-kr,HARE-512))

include ../api/link_finish.mk

.PHONY: reproduce-multi-instance
reproduce-multi-instance:
	@mkdir -p build
	@set -e; \
	R='$(IMPL)'; C="$$R/_shared/hare_core"; A="$$R/_shared/api_pkc"; \
	for L in 128 256 384 512; do \
	  $(CC) -O2 -std=gnu11 -w -DHARE_SYMMETRIC_MODE_B -DHARE_BACKEND_KR=1 \
	    -DHARE_INSTANCE_NAME=\"HARE-$$L-kr\" \
	    -I"$$R/Reference_Implementation/HARE-$$L/kr" -I"$$C/common" -I"$$C/ref" -I"$$A" \
	    reproduce_multi_instance_e2e.c "$$C"/common/*.c "$$C"/ref/*.c \
	    "$$A/auxfunc.c" "$$A/drng.c" \
	    "$$R/Reference_Implementation/HARE-$$L/kr/KEM_AlgorithmInstance.c" \
	    -o "build/reproduce_multi_instance_$$L" -lcrypto; \
	  "build/reproduce_multi_instance_$$L" 2; \
	done
