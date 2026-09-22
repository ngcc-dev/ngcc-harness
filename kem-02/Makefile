# NGCC uniform build: kem-02 Amoeba (reference implementation)
NGCC_ID   := kem-02
NGCC_TYPE := kem
NGCC_ALG  := Amoeba
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors

# Per src/CMakeLists.txt: symmetric/ + backend/ + api/ compiled with
# -DSECURITY_LEVEL=<bits> (set in each Amoeba-<n>/CMakeLists.txt); the five
# instance trees are otherwise identical copies. The candidate defines
# drng_algorithm itself in backend/cpapke.c (-> NGCC_NO_DRNG) and ships its own
# drng.c/auxfunc.c (byte-identical to api/). backend/randombytes.c (getrandom
# syscall) is only reached through CPAPKE_Init(), which the kem_* API never
# calls; it is compiled solely to satisfy the symbol reference.
# PATCH: backend/ntt_2s3t.c defines split/compose/radix2/radix3(inv) as bare
# C99 `inline` with no external definition, so at -O2 the calls are left
# unresolved (the candidate only links because its -O3 build inlines them).
# patches/common/ntt_2s3t.c is a copy with `static inline`; it is byte-identical
# in all five instance trees, hence one shared copy (path is physical: src/<label>
# resolves to Implementations/Reference_Implementation/Amoeba-<n>/src). A -Dinline trick is not
# possible because DEFS_/CFLAGS_ also reach the shim (static inline in api/).
AMOEBA_SRCS := api/KEM_AlgorithmInstance.c \
               backend/ccakem.c backend/cpapke.c backend/hamming.c ../../../../patches/common/ntt_2s3t.c \
               backend/poly.c backend/randombytes.c backend/zetas.c \
               symmetric/auxfunc.c symmetric/drng.c

# label, dir, SECURITY_LEVEL
define AMOEBA
SRCS_$(1) := $(AMOEBA_SRCS)
INC_$(1)  := -Isrc/$(1)/symmetric -Isrc/$(1)/backend -Isrc/$(1)/api
DEFS_$(1) := -DSECURITY_LEVEL=$(3)
SHIMDEFS_$(1) := -DNGCC_INSTANCE_HEADER='"api/KEM_AlgorithmInstance.h"' -DNGCC_NO_DRNG
$$(eval $$(call ngcc_instance,$(1),$(REF)/$(2)/src))
endef

$(eval $(call AMOEBA,Amoeba128,Amoeba-576,128))
$(eval $(call AMOEBA,Amoeba192,Amoeba-864,192))
$(eval $(call AMOEBA,Amoeba256,Amoeba-1152,256))
$(eval $(call AMOEBA,Amoeba384,Amoeba-1728,384))
$(eval $(call AMOEBA,Amoeba512,Amoeba-2304,512))

# Attack-only build: export the candidate's CPA encryption and hash helpers so
# a chosen-plaintext experiment can construct valid FO ciphertexts. The normal
# harness library remains limited by api/link_exports.map.
lib/libAmoeba128_attack.so: build/Amoeba128/shim.o $(OBJS_Amoeba128) | lib
	$(CC) -shared -Wl,-Bsymbolic -Wl,--no-undefined -Wl,-z,noexecstack -o $@ $^ -lm

include ../api/link_finish.mk

.PHONY: exploit
exploit:
	python3 reproduce_incomplete_compare.py

PYTHON ?= python3
.PHONY: exploit-key-recovery
exploit-key-recovery: lib/libAmoeba128_attack.so
	OPENBLAS_NUM_THREADS=4 $(PYTHON) recover_amoeba576.py --recover
