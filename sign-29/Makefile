# NGCC uniform build: sign-29 Tins (reference implementation)
NGCC_ID   := sign-29
NGCC_TYPE := sig
NGCC_ALG  := Tins
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Non-standard naming: the ICCS API implementation is SIG_TINS<level>.[ch]
# instead of SIG_AlgorithmInstance.[ch] (the header still defines
# ALGORITHM_INSTANCE). Source list per README: ff_arith.c SIG_TINS*.c drng.c
# bavc_commit.c auxfunc.c (the default glob picks exactly these; main.c and
# KAT_SIG.c are drivers). Randomness comes from drng_algorithm. The shipped
# drng.c differs from api/drng.c only in whitespace. A prebuilt `main` binary
# ships with each instance and is not used.
define TINS
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"SIG_TINS$(2).h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call TINS,Tins128,128))
$(eval $(call TINS,Tins256,256))
$(eval $(call TINS,Tins512,512))

include ../api/link_finish.mk

TINS128_SRC := $(REF)/Tins128

.PHONY: exploit clean-exploit
exploit: recover_tins128
	./recover_tins128

recover_tins128: recover_tins128.c \
	$(TINS128_SRC)/SIG_TINS128.c $(TINS128_SRC)/bavc_commit.c \
	$(TINS128_SRC)/ff_arith.c $(TINS128_SRC)/auxfunc.c $(TINS128_SRC)/drng.c
	$(CC) -O2 -std=c99 -Wall -Wextra -I$(TINS128_SRC) $^ -o $@

clean-exploit:
	rm -f recover_tins128
