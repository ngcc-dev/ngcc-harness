# Supplementary XOF instances for the output-length test (supplement.py): each
# library exposes CryptHash() at any byte-aligned output length.
#
#   pseudoXOF             ICCS pseudoXOF (KDF-SM3), api/auxfunc.c unchanged
#   SHAKE128, SHAKE256    pq-crystals fips202.c (../upstream/kyber/ref)
#   CHash-LDXOF-512/1024  hash-03 C Hash's Lane-Dedicated XOF, from the submitted
#                         reference code but outside its ICCS interface
#   Laurus-XOF-adapted,   hash-14 / hash-15 submitted XOF functions with the
#   litchi_xof-adapted    requested length passed through (their CryptHash
#                         fixes it at DIGEST_BIT_LENGTH)
#
# The candidates' own XOF instances are built by their harness Makefiles.
NGCC_ROOT := ../../..
NGCC_ID   := xof-ref
NGCC_TYPE := hash
NGCC_ALG  := XOF reference points
include $(NGCC_ROOT)/api/link_rules.mk

# source paths are relative to src/<label>, the link to ../adapter
KYBER := ../upstream/kyber/ref

SRCS_pseudoXOF := xof_hash.c ../../../api/auxfunc.c
DEFS_pseudoXOF := -DXOF_PSEUDOXOF
SHIMDEFS_pseudoXOF := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"pseudoXOF"' -DNGCC_DIGEST_BITS=256
$(eval $(call ngcc_instance,pseudoXOF,../adapter))

# $(1)=label $(2)=XOF_SHAKE128|XOF_SHAKE256
define SHAKE
SRCS_$(1) := xof_hash.c $(KYBER)/fips202.c
INC_$(1)  := -I$(KYBER)
DEFS_$(1) := -D$(2)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"$(1)"' -DNGCC_DIGEST_BITS=256
$$(eval $$(call ngcc_instance,$(1),../adapter))
endef
$(eval $(call SHAKE,SHAKE128,XOF_SHAKE128))
$(eval $(call SHAKE,SHAKE256,XOF_SHAKE256))

# the C Hash reference directory has spaces in its path; link it without them
$(shell mkdir -p src && ln -sfn '$(CURDIR)/../../../hash-03/C Hash/Implementations/Reference_Implementation' src/chash-ref)

# $(1)=label $(2)=512|1024
define LDXOF
SRCS_$(1) := chash_ldxof.c
INC_$(1)  := -Isrc/chash-ref/CHash_$(2)
DEFS_$(1) := -DCHASH_LDXOF_VARIANT=$(2)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"$(1)"' -DNGCC_DIGEST_BITS=$(2)
$$(eval $$(call ngcc_instance,$(1),../adapter))
endef
$(eval $(call LDXOF,CHash-LDXOF-512,512))
$(eval $(call LDXOF,CHash-LDXOF-1024,1024))

# Laurus-XOF (hash-14) and litchi_xof (hash-15): the submitted CryptHash always
# returns DIGEST_BIT_LENGTH bits; these pass the requested length to the
# submitted XOF function instead (adapter/fixed_xof.c)
$(shell ln -sfn '$(CURDIR)/../../../hash-14/Laurus/Implementations/Reference_Implementation/Laurus-XOF' src/laurus-xof)
$(shell ln -sfn '$(CURDIR)/../../../hash-15/Litchi/Implementations/Reference_Implementation/Litchi-XOF' src/litchi-xof)

# $(1)=label $(2)=link under src/ $(3)=XOF source file $(4)=XOF function
define ADAPTED
SRCS_$(1) := fixed_xof.c ../xof/src/$(2)/$(3)
INC_$(1)  := -Isrc/$(2)
DEFS_$(1) := -DXOF_FUNCTION=$(4)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"$(1)"' -DNGCC_DIGEST_BITS=256
$$(eval $$(call ngcc_instance,$(1),../adapter))
endef
$(eval $(call ADAPTED,Laurus-XOF-adapted,laurus-xof,laurus_xof.c,laurus_xof))
$(eval $(call ADAPTED,litchi_xof-adapted,litchi-xof,litchi_xof.c,litchi_xof))

include $(NGCC_ROOT)/api/link_finish.mk
