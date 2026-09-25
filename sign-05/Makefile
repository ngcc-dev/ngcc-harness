# NGCC uniform build: sign-05 Chinith (reference implementation)
NGCC_ID   := sign-05
NGCC_TYPE := sig
NGCC_ALG  := Chinith
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# VOLE-in-the-head family (FAEST-like) with three OWF backends: SM4 (sm4th_*),
# uBlock (ublockith_*) and Vistrutah (vistrutith_*); "tight" variants add the
# portable Ballet-256/256 PRG core. SOURCES/CFLAGS are copied from each
# instance's Makefile (minus KAT_SIG.c/bench). The API header is named
# <instance>_AlgorithmInstance.h, hence NGCC_INSTANCE_HEADER. The candidate's
# `-include fallbacks.h` (static inline explicit_bzero/mempcpy for -std=c11)
# is not used: with the harness's -D_GNU_SOURCE glibc's string.h provides both
# and the static definitions would clash. Randomness: the AlgorithmInstance.c
# draws from drng_algorithm directly. drng.c/auxfunc.h differ from the official
# copies only by trailing comments. -DNDEBUG is the candidate's default
# (NDEBUG=1) and is required: vole.c has an assert() on an undeclared variable
# that only compiles away with NDEBUG.
CH_COMMON := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c \
             universal_hashing.c utils.c vole.c
SM4_SRCS  := owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c utils_sm4/sm4_core.c \
             utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c
BALLET    := utils_ballet/Ballet256256_coreE.c
UBLOCK_SRCS := owf.c utils_ublock/sig_impl_ublock.c utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c \
               utils_ublock/ublock_core.c utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c \
               utils_ublock/ublockith_ublock_256.c
VIS_SRCS  := utils_vistrutah/sig_impl_vistrutith.c utils_vistrutah/vistrutah.c utils_vistrutah/vistrutith_constraints.c \
             utils_vistrutah/vistrutith_vistrutah_512.c utils_vistrutah/vistrutith_witness.c

# label, extra sources, include dirs (relative to instance), defines
define CH
SRCS_$(1) := $(CH_COMMON) $(2) $(1).c $(1)_AlgorithmInstance.c
INC_$(1)  := $(addprefix -Isrc/$(1)/,$(3))
DEFS_$(1) := -DNDEBUG $(4)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"$(1)_AlgorithmInstance.h"'
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

# sm4th d3 variants also build sig_timing.c (optional component timing hooks)
$(eval $(call CH,sm4th_d3_128f_loose,$(SM4_SRCS) sig_timing.c,utils_sm4,-DSM4_SBOX_TABLE))
$(eval $(call CH,sm4th_d3_128s_loose,$(SM4_SRCS) sig_timing.c,utils_sm4,-DSM4_SBOX_TABLE))
$(eval $(call CH,sm4th_d3_128f_tight,$(SM4_SRCS) sig_timing.c $(BALLET),utils_sm4 utils_ballet,-DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,sm4th_d3_128s_tight,$(SM4_SRCS) sig_timing.c $(BALLET),utils_sm4 utils_ballet,-DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,sm4th_em_d2_128f_loose,$(SM4_SRCS),utils_sm4,-DSM4_SBOX_TABLE))
$(eval $(call CH,sm4th_em_d2_128s_loose,$(SM4_SRCS),utils_sm4,-DSM4_SBOX_TABLE))
$(eval $(call CH,sm4th_em_d2_128f_tight,$(SM4_SRCS) $(BALLET),utils_sm4 utils_ballet,-DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,sm4th_em_d2_128s_tight,$(SM4_SRCS) $(BALLET),utils_sm4 utils_ballet,-DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,ublockith_d3_256f,$(UBLOCK_SRCS) $(BALLET),utils_ublock utils_ballet,-DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,ublockith_d3_256s,$(UBLOCK_SRCS) $(BALLET),utils_ublock utils_ballet,-DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,ublockith_em_d3_256f,$(UBLOCK_SRCS) $(BALLET),utils_ublock utils_ballet,-DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,ublockith_em_d3_256s,$(UBLOCK_SRCS) $(BALLET),utils_ublock utils_ballet,-DUSE_BALLET=1 -DHAVE_BALLET_CORE=1))
$(eval $(call CH,vistrutith_d3_512f,$(VIS_SRCS),utils_vistrutah,-DSBOX_TABLE))
$(eval $(call CH,vistrutith_d3_512s,$(VIS_SRCS),utils_vistrutah,-DSBOX_TABLE))

# sign-05-1 witness: the instance's own signer on a false witness, statically
# linked with that instance's objects (the unpacked signer is not exported).
CH_ALL := sm4th_d3_128f_loose sm4th_d3_128s_loose sm4th_d3_128f_tight sm4th_d3_128s_tight \
          sm4th_em_d2_128f_loose sm4th_em_d2_128s_loose sm4th_em_d2_128f_tight sm4th_em_d2_128s_tight \
          ublockith_d3_256f ublockith_d3_256s ublockith_em_d3_256f ublockith_em_d3_256s \
          vistrutith_d3_512f vistrutith_d3_512s

bin:
	mkdir -p $@

define forgery_target
bin/reproduce_forgery_$(1): reproduce_forgery.c build/$(1)/shim.o $$(OBJS_$(1)) | bin
	$(CC) $(NGCC_CFLAGS) $$(DEFS_$(1)) $$(INCFLAGS_$(1)) -DINST=$(1) -o $$@ $$^ $(NGCC_LDLIBS)
endef
$(foreach i,$(CH_ALL),$(eval $(call forgery_target,$(i))))

.PHONY: exploit reproduce clean-witness
exploit: $(addprefix bin/reproduce_forgery_,$(CH_ALL))

reproduce: exploit
	@set -e; for i in $(CH_ALL); do ./bin/reproduce_forgery_$$i; done

clean: clean-witness
clean-witness:
	rm -rf bin

include ../api/link_finish.mk
