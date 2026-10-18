# Explicit reviewed reference source lists; no submitted scripts or prebuilts.
.DEFAULT_GOAL := all
CC ?= cc
CFLAGS ?= -O2
KAT_LIMIT ?= 0
REF := submission/SQIsign/Reference_Implementation
SRC := $(REF)/src
INSTANCES := p324_3 p500_27 p664_17
MODULES := mp gf ec hd verification signature id2iso quaternion
SOURCES := mp/ref/lvlx/mp.c mp/ref/lvlx/modqx.c gf/ref/lvlx/fp.c gf/ref/lvlx/fp2.c ec/ref/lvlx/ec.c ec/ref/lvlx/isog.c ec/ref/lvlx/basis.c ec/ref/lvlx/biextension.c ec/ref/lvlx/normalize.c hd/ref/lvlx/hd.c hd/ref/lvlx/theta_structure.c hd/ref/lvlx/gluing.c hd/ref/lvlx/splitting.c hd/ref/lvlx/theta_isogenies.c verification/ref/lvlx/common.c verification/ref/lvlx/encode_public.c verification/ref/lvlx/verify.c signature/ref/lvlx/encode_secret.c signature/ref/lvlx/keygen.c signature/ref/lvlx/sign.c id2iso/ref/lvlx/id2iso.c id2iso/ref/lvlx/dim2id2iso.c quaternion/ref/lvlx/algebra.c quaternion/ref/lvlx/dim4.c quaternion/ref/lvlx/dim2.c quaternion/ref/lvlx/integers.c quaternion/ref/lvlx/lattice.c quaternion/ref/lvlx/finit.c quaternion/ref/lvlx/printer.c quaternion/ref/lvlx/lll_applications.c quaternion/ref/lvlx/lll/lll_lg2.c quaternion/ref/lvlx/lll/lll_dim4.c quaternion/ref/lvlx/lll/lehmer_xgcd.c quaternion/ref/lvlx/lll/gaussian_xgcd.c quaternion/ref/lvlx/stdorder.c quaternion/ref/lvlx/qlapoty.c quaternion/ref/lvlx/ideal.c quaternion/ref/lvlx/protocol.c quaternion/ref/lvlx/test/random_input_generation.c common/generic/randombytes_system.c common/generic/fips202.c common/generic/mem.c common/generic/prng.c common/generic/tools.c common/ref/randombytes_ctrdrbg.c common/ref/aes_c.c sqisign.c
PRECOMP := ec_params.c e0_basis.c torsion_constants.c quaternion_data.c endomorphism_action.c
all: $(addprefix lib/,$(addsuffix .so,$(INSTANCES)))
define instance
SRCS_$(1) := $$(addprefix $$(SRC)/,$$(SOURCES)) $$(addprefix $$(SRC)/precomp/ref/$(1)/,$$(PRECOMP)) $$(SRC)/gf/ref/$(1)/fp_$(1).c $$(SRC)/nistapi/$(1)/api.c
INCS_$(1) := $$(REF)/include $$(SRC)/common/generic/include $$(SRC)/common/ref/include $$(SRC)/precomp/ref/$(1)/include $$(SRC)/gf/ref/$(1)/include $$(SRC)/ec/ref/$(1)/include $$(SRC)/nistapi/$(1) $$(SRC)/quaternion/ref/lvlx/internal_quaternion_headers $$(foreach m,$$(MODULES),$$(SRC)/$$(m)/ref/include)
lib/$(1).so: $$(SRCS_$(1)) $$(shell find $$(REF)/include $$(SRC) -name '*.h') ../api/nist_sig_shim.c ../api/nist_sig_api.h Makefile
	mkdir -p lib
	$$(CC) $$(CFLAGS) -std=c11 -fPIC -shared -Wl,-Bsymbolic -fno-strict-aliasing -DNDEBUG -DENABLE_SIGN -DRANDOMBYTES_C -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DSQISIGN_VARIANT=$(1) -DNIST_SIG_SCHEME='"SQIsign"' -DNIST_SIG_PARAMETER='"$(1)"' $$(addprefix -I,$$(INCS_$(1))) -I../api $$(SRCS_$(1)) ../api/nist_sig_shim.c -lm -o $$@
endef
$(foreach p,$(INSTANCES),$(eval $(call instance,$(p))))
.PHONY: all libs test clean
libs: all
test: libs ../bin/nist-sig
	KAT_LIMIT=$(KAT_LIMIT) python3 run_tests.py
clean:
	rm -rf lib results

../bin/nist-sig:
	$(MAKE) -C ../api harness

.PHONY: manifest check-results $(addprefix test-,$(INSTANCES))
manifest:
	python3 ../tools/regenerate_manifests.py SQIsign
check-results:
	python3 ../tools/run_library_kat.py --scheme SQIsign --check-summary --limit $(KAT_LIMIT)
$(addprefix test-,$(INSTANCES)): test-%: lib/%.so ../bin/nist-sig
	python3 ../tools/run_library_kat.py --scheme SQIsign --parameter $* --library lib/$*.so --manifest kat/$*.tsv --limit $(KAT_LIMIT)
