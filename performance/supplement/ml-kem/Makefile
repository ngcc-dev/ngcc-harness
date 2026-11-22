# Supplementary reference instances: ML-KEM (FIPS 203), pq-crystals reference
# implementation (../upstream/kyber, see ../PROVENANCE.md), built and linked
# exactly like a candidate (api/link_rules.mk) behind the ICCS KEM API. Not an
# NGCC candidate; timed by performance/supplement/supplement.py.
#
#   make libs              lib/libML-KEM-{512,768,1024}.so
#   make tvecs             upstream deterministic test-vector programs, same flags
NGCC_ROOT := ../../..
NGCC_ID   := ml-kem
NGCC_TYPE := kem
NGCC_ALG  := ML-KEM
include $(NGCC_ROOT)/api/link_rules.mk

UP := ../upstream/kyber/ref
UPSRCS := cbd.c fips202.c indcpa.c kem.c ntt.c poly.c polyvec.c reduce.c symmetric-shake.c verify.c
# paths relative to src/<label>, the link to $(UP); upstream randombytes.c
# (/dev/urandom) is replaced by the ICCS DRNG
ADAPTER := ../../../adapter

# $(1)=label $(2)=KYBER_K
define MLKEM
SRCS_$(1) := $(UPSRCS) $(ADAPTER)/mlkem_iccs.c $(ADAPTER)/randombytes_drng.c
INC_$(1)  := -I../adapter
DEFS_$(1) := -DKYBER_K=$(2)
NOAUX_$(1) := 1
SHIMDEFS_$(1) := -DNGCC_INSTANCE='"$(1)"' -DNGCC_INSTANCE_HEADER='"iccs_api.h"'
$$(eval $$(call ngcc_instance,$(1),$(UP)))

tvecs/$(1): $(addprefix $(UP)/,$(UPSRCS)) $(UP)/test/test_vectors.c | tvecs
	$(CC) $(NGCC_CFLAGS) -DKYBER_K=$(2) -o $$@ $(addprefix $(UP)/,$(UPSRCS)) $(UP)/test/test_vectors.c
TVECS += tvecs/$(1)
endef

$(eval $(call MLKEM,ML-KEM-512,2))
$(eval $(call MLKEM,ML-KEM-768,3))
$(eval $(call MLKEM,ML-KEM-1024,4))

tvecs:
	mkdir -p $@
tvecs-all: $(TVECS)
.PHONY: tvecs-all

include $(NGCC_ROOT)/api/link_finish.mk
clean: clean-tvecs
clean-tvecs:
	rm -rf tvecs
.PHONY: clean-tvecs
