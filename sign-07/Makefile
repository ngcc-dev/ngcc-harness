# NGCC uniform build: sign-07 CS (reference implementation)
NGCC_ID   := sign-07
NGCC_TYPE := sig
NGCC_ALG  := CS
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# No candidate Makefile: README says `gcc *.c -o main -O2`, i.e. every .c in
# the directory (= default glob minus KAT_SIG.c). The three directories differ
# only in `#define lambda` in params.h, which also sets ALGORITHM_NAME
# ("CS_128" etc.) used as ALGORITHM_INSTANCE, matching KAT_SIG_CS_<n>.txt.
# Randomness is drawn from drng_algorithm in cs.c; drng.c/auxfunc.c are the
# unmodified official copies.
#
# The candidate's SIG_AlgorithmInstance.h includes cs.h -> params.h, which
# #defines single-letter macros (n, k, q, ...) that break the shim's own
# `size_t n` parameter. The shim therefore includes the official ICCS API
# template header instead (identical prototypes, no params.h) and gets the
# instance name via -DNGCC_INSTANCE.
#
# ntt.c does #include "NTT.h", which is not in the submission (only ntt.h
# is, and it declares MultiplyNTT non-static while ntt.c defines it static,
# so ntt.c cannot compile against ntt.h on any platform). patches/common/NTT.h
# is a stand-in that includes ntt.h with that one prototype renamed; no
# submission file is modified. Likely the submitters' real NTT.h was lost
# (case-insensitive filesystem, .DS_Store present).
#
# symmetric.h holds a tentative definition `DRNG_ctx drng_algorithm;` and is
# included by cs.c, sampling.c and SIG_AlgorithmInstance.c (KAT_SIG.c defines
# it once more), so the candidate defines the DRNG context itself and relies
# on common-symbol merging: -fcommon (GCC >= 10 defaults to -fno-common) and
# -DNGCC_NO_DRNG so the shim only references it.
ICCS_SIG_H := API_PKC/Implementations/Reference_Implementation/AlgorithmInstance/SIG_AlgorithmInstance.h

# label, instance name
define CS
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"$(ICCS_SIG_H)"' -DNGCC_INSTANCE='"$(2)"' -DNGCC_NO_DRNG
INC_$(1) := -Ipatches/common
CFLAGS_$(1) := -fcommon -w
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call CS,CS-128,CS_128))
$(eval $(call CS,CS-256,CS_256))
$(eval $(call CS,CS-512,CS_512))

# ------------------------------------------------------------------ exploit
#
# forgery.c is a universal forgery witness: CS_Verify uses the
# challenge only through c mod 2, so the tau sign bits of the {0,+-1} challenge
# are invisible to it, and the unbounded hint plus the alpha/4 term inside the
# z2' bound let a forger write down a transcript for any challenge support with
# z0 = z1 = 0.  See the header of forgery.c.
#
# The driver is built once per parameter set, against that set's own params.h,
# and linked against the submission's own sources (the library itself exports
# only the official API, so the internal encoders are not reachable through
# it).  Key generation and the final verdict still go through the dlopened
# library's official API.
FORGE_SRCS   := auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c \
                ntt.c sampling.c
FORGE_CFLAGS := -O2 -std=gnu11 -fcommon -w $(NGCC_PERMISSIVE) \
                -I$(API) -Ipatches/common
FORGE_LDLIBS := -ldl -lm -lpthread
FORGE_BINS   := $(foreach i,CS-128 CS-256 CS-512,forgery_$(i))

define forge_rule
forgery_$(1): forgery.c $$(addprefix $(REF)/$(1)/,$$(FORGE_SRCS)) \
                        $(API)/link_common.h $(API)/link_sig.h
	$$(CC) $$(FORGE_CFLAGS) -I$(REF)/$(1) -o $$@ forgery.c \
		$$(addprefix $(REF)/$(1)/,$$(FORGE_SRCS)) $$(FORGE_LDLIBS)
endef
$(foreach i,CS-128 CS-256 CS-512,$(eval $(call forge_rule,$(i))))

# Scaled-down instance: the CS-128 sources compiled against
# patches/scaled-tau3/params.h, whose only substantive change is tau 23 -> 3.
# The forgery costs C(n,tau) hash calls, so this is the same attack at a
# runnable size; the submitted sets are covered by the control run, which uses
# the identical code and trial budget and finds nothing.
#
# No submission file is edited or copied.  build/<scaled>/src is a symlink farm
# over the CS-128 directory in which params.h points at the patched header
# instead; because the candidate sources reach params.h through
# `#include "params.h"` from ntt.h, and a quoted include resolves against the
# directory of the *including* file, compiling the symlinked sources picks up
# the patched header and nothing else changes.  (-include cannot be used: it
# would define the single-letter macros before the system headers.)
SCALED       := CS-128-scaled-tau3
SCALED_SRC   := $(REF)/CS-128
SCALED_PARAMS:= patches/scaled-tau3/params.h
SCALED_FARM  := build/$(SCALED)/src
SCALED_STAMP := $(SCALED_FARM)/.stamp
SCALED_OBJS  := $(patsubst %.c,build/$(SCALED)/%.o,$(FORGE_SRCS)) \
                build/$(SCALED)/SIG_AlgorithmInstance.o

$(SCALED_STAMP): $(SCALED_PARAMS) patches/common/NTT.h
	@mkdir -p $(SCALED_FARM)
	@cd $(SCALED_FARM) && rm -f *.c *.h && \
	  for f in $(CURDIR)/$(SCALED_SRC)/*.c $(CURDIR)/$(SCALED_SRC)/*.h; do \
	    case $$(basename $$f) in params.h) ;; *) ln -sf "$$f" . ;; esac; done && \
	  ln -sf $(CURDIR)/$(SCALED_PARAMS) params.h && \
	  ln -sf $(CURDIR)/patches/common/NTT.h NTT.h
	@touch $@

build/$(SCALED)/%.o: $(SCALED_FARM)/%.c $(SCALED_STAMP)
	$(CC) $(NGCC_CFLAGS) -fcommon -w -I$(SCALED_FARM) -I$(API) -c -o $@ $<

# The shim must NOT see params.h: its single-letter macros collide with the
# shim's own parameter names (see the note above).
build/$(SCALED)/shim.o: $(API)/link_shim.c $(SCALED_STAMP)
	$(CC) $(NGCC_CFLAGS) -fcommon -w -I$(API) $(NGCC_TYPE_DEF) \
		-DNGCC_ID='"$(NGCC_ID)"' -DNGCC_ALG='"$(NGCC_ALG)"' \
		-DNGCC_VARIANT='"Reference_Implementation"' \
		-DNGCC_SRCDIR='"$(SCALED_SRC)"' -DNGCC_FLAGS='"-O2 scaled tau=3"' \
		-DNGCC_INSTANCE_HEADER='"$(ICCS_SIG_H)"' \
		-DNGCC_INSTANCE='"$(SCALED)"' -DNGCC_NO_DRNG -c -o $@ $<

lib/lib$(SCALED).so: build/$(SCALED)/shim.o $(SCALED_OBJS) | lib
	$(CC) $(NGCC_LDFLAGS) -o $@ $^ $(NGCC_LDLIBS)

forgery_$(SCALED): forgery.c $(SCALED_STAMP) \
                             $(API)/link_common.h $(API)/link_sig.h
	$(CC) $(FORGE_CFLAGS) -I$(SCALED_FARM) -o $@ forgery.c \
		$(addprefix $(SCALED_FARM)/,$(FORGE_SRCS)) $(FORGE_LDLIBS)

.PHONY: exploit clean-exploit
all: $(FORGE_BINS) forgery_$(SCALED) lib/lib$(SCALED).so
exploit: $(FORGE_BINS) forgery_$(SCALED) lib/lib$(SCALED).so

clean-exploit:
	rm -f $(FORGE_BINS) forgery_$(SCALED)

include ../api/link_finish.mk
