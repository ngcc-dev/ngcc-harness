# NGCC uniform build: kex-02 AFS-KEX (reference implementation)
NGCC_ID   := kex-02
NGCC_TYPE := kex
NGCC_ALG  := AFS-KEX
include ../api/link_rules.mk

REF := Implementations and Test_Vectors/Implementations/Reference_Implementation
NGCC_KAT_DIR := Implementations and Test_Vectors/Test_Vector

# Source list (CORE_SRC + API_SRC) and CPPFLAGS from each instance's Makefile.
# Additional_Implementation/ARM_Cortex_M4 (mupq) is ignored. drng.c/auxfunc.c
# are the official files (CRLF line endings only). KEX_AlgorithmInstance.c
# draws from drng_algorithm and only calls the *_derand KEM entry points;
# randombytes_api.c (getrandom(2), OS entropy) is linked because kem.c's
# non-derand crypto_kem_keypair/enc reference it, but it is never reached.
# NOTE: this candidate claims kex_get_passes_num() == 4 (M4_Len = 0 in the
# KATs) and exports kex_generate_pass4_msg_b; the harness supports 1-3 passes.
AFS_SRCS := BWcoding.c cbd.c indcpa.c kem.c ntt.c poly.c polyvec.c randombytes_api.c reduce.c \
            symmetric-iccs.c verify.c auxfunc.c drng.c KEX_AlgorithmInstance.c

# label, extra defines
define AFS
SRCS_$(1) := $(AFS_SRCS)
DEFS_$(1) := $(2)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call AFS,AFS_KEX_C128,-DBWKEM128_INTERNAL_COMPAT -DBWKEM_C128_USE_ICCS_AUXFUNC))
$(eval $(call AFS,AFS_KEX_C256,-DBWKEM_C256_USE_ICCS_AUXFUNC))
$(eval $(call AFS,AFS_KEX_C512,-DBWKEM_C512_USE_ICCS_AUXFUNC))

.PHONY: exploit clean-exploit reproduce-protocol-findings
all: reproduce_pfs_break reproduce_pfs_break_c256 reproduce_pfs_break_c512
exploit: reproduce_pfs_break reproduce_pfs_break_c256 reproduce_pfs_break_c512

reproduce_pfs_break: reproduce_pfs_break.c $(OBJS_AFS_KEX_C128)
	$(CC) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter \
		-DBWKEM128_INTERNAL_COMPAT -DBWKEM_C128_USE_ICCS_AUXFUNC \
		-Isrc/AFS_KEX_C128 -o $@ reproduce_pfs_break.c $(OBJS_AFS_KEX_C128) -lm

reproduce_pfs_break_c256: reproduce_pfs_break.c $(OBJS_AFS_KEX_C256)
	$(CC) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter \
		-DBWKEM_C256_USE_ICCS_AUXFUNC -Isrc/AFS_KEX_C256 \
		-o $@ reproduce_pfs_break.c $(OBJS_AFS_KEX_C256) -lm

reproduce_pfs_break_c512: reproduce_pfs_break.c $(OBJS_AFS_KEX_C512)
	$(CC) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter \
		-DBWKEM_C512_USE_ICCS_AUXFUNC -Isrc/AFS_KEX_C512 \
		-o $@ reproduce_pfs_break.c $(OBJS_AFS_KEX_C512) -lm

clean-exploit:
	rm -f reproduce_pfs_break reproduce_pfs_break_c256 reproduce_pfs_break_c512
clean: clean-exploit

reproduce-protocol-findings:
	@./reproduce_protocol_findings.sh

include ../api/link_finish.mk
