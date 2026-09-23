# NGCC uniform build: hash-01 AFS-TrEDM (reference implementation)
NGCC_ID   := hash-01
NGCC_TYPE := hash
NGCC_ALG  := AFS-TrEDM
include ../api/link_rules.mk

REF := AFS-TrEDM/Implementations/Reference_Implementation
NGCC_KAT_DIR := AFS-TrEDM/Test_Vectors

# Source list from the candidate Makefile (KAT_SRCS minus the KAT driver).
# The directory also holds several stand-alone programs with main()
# (hash_cli.c, profile_info.c, benchmark.c, quick_test.c) that the default
# glob would not all drop, hence the explicit list. drng.c is byte-identical
# to api/drng.c.
AFS_SRCS := CryptHash_AlgorithmInstance.c afs_sbox64.c afs_lmds1600_s6.c afs_p1600.c afs_tredm.c drng.c

define AFS
SRCS_$(1) := $(AFS_SRCS)
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(1)))
endef

$(eval $(call AFS,AFS-TrEDM-512))
$(eval $(call AFS,AFS-TrEDM-768))
$(eval $(call AFS,AFS-TrEDM-1024))

include ../api/link_finish.mk
