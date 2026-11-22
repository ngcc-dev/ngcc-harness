# Supplementary reference instances: ML-DSA (FIPS 204), pq-crystals reference
# implementation (../upstream/dilithium, see ../PROVENANCE.md), built and linked
# exactly like a candidate (api/link_rules.mk) behind the ICCS signature API.
# Not an NGCC candidate; timed by performance/supplement/supplement.py.
#
#   make libs              lib/libML-DSA-{44,65,87}.so
#   make tvecs             upstream deterministic test-vector programs, same flags
NGCC_ROOT := ../../..
NGCC_ID   := ml-dsa
NGCC_TYPE := sig
NGCC_ALG  := ML-DSA
include $(NGCC_ROOT)/api/link_rules.mk

UP := ../upstream/dilithium/ref
UPSRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c fips202.c symmetric-shake.c
ADAPTER := ../../../adapter

# $(1)=label $(2)=DILITHIUM_MODE
define MLDSA
SRCS_$(1) := $(UPSRCS) $(ADAPTER)/mldsa_iccs.c $(ADAPTER)/randombytes_drng.c
INC_$(1)  := -I../adapter
DEFS_$(1) := -DDILITHIUM_MODE=$(2)
NOAUX_$(1) := 1
SHIMDEFS_$(1) := -DNGCC_INSTANCE='"$(1)"' -DNGCC_INSTANCE_HEADER='"iccs_api.h"'
$$(eval $$(call ngcc_instance,$(1),$(UP)))

tvecs/$(1): $(addprefix $(UP)/,$(UPSRCS)) $(UP)/test/test_vectors.c | tvecs
	$(CC) $(NGCC_CFLAGS) -DDILITHIUM_MODE=$(2) -o $$@ $(addprefix $(UP)/,$(UPSRCS)) $(UP)/test/test_vectors.c
TVECS += tvecs/$(1)
endef

$(eval $(call MLDSA,ML-DSA-44,2))
$(eval $(call MLDSA,ML-DSA-65,3))
$(eval $(call MLDSA,ML-DSA-87,5))

tvecs:
	mkdir -p $@
tvecs-all: $(TVECS)
.PHONY: tvecs-all

include $(NGCC_ROOT)/api/link_finish.mk
clean: clean-tvecs
clean-tvecs:
	rm -rf tvecs
.PHONY: clean-tvecs
