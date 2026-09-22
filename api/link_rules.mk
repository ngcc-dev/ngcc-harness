# NGCC uniform build rules. Included by every candidate Makefile.
#
# A candidate Makefile looks like:
#
#   NGCC_ID   := kex-07
#   NGCC_TYPE := kex            # kem | sig | kex | hash
#   NGCC_ALG  := NEV-AKE
#   include ../api/link_rules.mk
#   $(eval $(call ngcc_instance,NEV-AKE-512,Implementations and Test_Vectors/Implementations/Reference_Implementation/NEV-AKE-512))
#   $(eval $(call ngcc_instance,NEV-AKE-768,...))
#   include ../api/link_finish.mk
#
# ngcc_instance,<label>,<source dir relative to the candidate folder>
#   Builds lib/lib<label>.so from the C/C++/asm sources in that directory.
#   The directory is symlinked as src/<label> so paths with spaces never reach
#   make's pattern functions. Only sources are compiled: nothing shipped in the
#   submission (binaries, objects, scripts, Makefiles) is executed or linked.
#
# Per-instance overrides, set BEFORE the $(eval $(call ngcc_instance,...)):
#   SRCS_<label>     explicit source list, relative to src/<label> (default:
#                    every *.c *.cpp *.cc *.S directly in the dir, minus the
#                    driver/benchmark files matched by NGCC_EXCLUDE_RE)
#   INC_<label>      extra -I flags (paths relative to the candidate folder)
#   DEFS_<label>     extra -D flags
#   CFLAGS_<label>   extra C flags
#   CXXFLAGS_<label> extra C++ flags
#   LDLIBS_<label>   extra libraries (e.g. -lgmp -lcrypto)
#   SHIMDEFS_<label> extra -D flags for the shim only (e.g. -DNGCC_INSTANCE="x",
#                    -DNGCC_DIGEST_BITS=512, -DNGCC_INSTANCE_HEADER=..., -DNGCC_NO_DRNG)
#   NOAUX_<label>    1 = do not add api/auxfunc.c even if the dir has none
#   NODRNG_<label>   1 = do not add api/drng.c even if the dir has none
#   KATDIR_<label>   directory holding this instance's KAT_*.txt files
#                    (default: first Test_Vector* directory in the candidate folder)
#   KATNAME_<label>  name part of KAT_<TYPE>_<name>.txt if it differs from the
#                    instance's ALGORITHM_INSTANCE
#   TESTFLAGS_<label> extra harness flags (e.g. --full)
#
# Candidate-wide overrides: NGCC_CFLAGS, NGCC_CXXFLAGS, NGCC_LDLIBS,
# NGCC_KAT_DIR, NGCC_TEST_INSTANCES (when not every built instance shipped a KAT).
#
# Targets: all (default) | test | manifest | clean | list
# The candidate Makefile must end with:  include $(API)/link_finish.mk

NGCC_ROOT ?= ..
API       := $(NGCC_ROOT)/api
HARNESS   := $(NGCC_ROOT)/bin/ngcc_kat

CC       ?= gcc
CXX      ?= g++
NGCC_OPT ?= -O2
NGCC_CFLAGS   ?= $(NGCC_OPT) -fPIC -std=gnu11 -Wall -Wno-unused -Wno-unknown-pragmas -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer -D_GNU_SOURCE $(NGCC_PERMISSIVE)
NGCC_CXXFLAGS ?= $(NGCC_OPT) -fPIC -std=gnu++17 -Wall -Wno-unused -Wno-unknown-pragmas -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer -D_GNU_SOURCE
NGCC_LDFLAGS  ?= -shared -Wl,--version-script=$(API)/link_exports.map -Wl,-Bsymbolic -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro
NGCC_LDLIBS   ?= -lm
# GCC 14+ makes these errors; many submissions were written against older
# compilers, so downgrade them to warnings (they are still printed).
NGCC_PERMISSIVE ?= -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-int
# basenames matching this (extended regex) are dropped from the default glob
NGCC_EXCLUDE_RE ?= ^(KAT_|PQCgenKAT|main|test|bench|speed|cpucycles|osfreq|run_bench|selftest|self_test|example|demo|kat\.c|kat_)|(_test|test_|_bench|bench_|_main|_kat)\.

NGCC_TYPEU := $(shell echo $(NGCC_TYPE) | tr a-z A-Z)
ifeq ($(NGCC_TYPE),hash)
NGCC_TYPE_DEF := -DNGCC_BUILD_HASH
else ifeq ($(NGCC_TYPE),kem)
NGCC_TYPE_DEF := -DNGCC_BUILD_KEM
else ifeq ($(NGCC_TYPE),sig)
NGCC_TYPE_DEF := -DNGCC_BUILD_SIG
else ifeq ($(NGCC_TYPE),kex)
NGCC_TYPE_DEF := -DNGCC_BUILD_KEX
else
$(error NGCC_TYPE must be one of kem sig kex hash)
endif

# canonical SHA-256 digests of the reference KAT files (see api/README.md);
# when present, `make test` verifies against it and the reference files are
# not needed
NGCC_MANIFEST ?= kat.sha256

NGCC_KAT_DIR ?= $(shell find . -maxdepth 4 -type d -iname 'Test_Vector*' -not -path '*/Others/*' 2>/dev/null | head -1)

INSTANCES :=
LIBS      :=

.PHONY: all test clean list
all: libs

# $(1)=label $(2)=source dir (relative to the candidate folder, may contain spaces)
define ngcc_instance
INSTANCES += $(1)
LIBS      += lib/lib$(1).so
SRCDIR_$(1) := $(2)
# symlink so that make never sees the spaces
$$(shell mkdir -p src && ln -sfn '$(CURDIR)/$(2)' 'src/$(1)')
ifeq ($$(origin SRCS_$(1)),undefined)
SRCS_$(1) := $$(shell cd 'src/$(1)' 2>/dev/null && ls -1 2>/dev/null | grep -E '\.(c|cpp|cc|S)$$$$' | grep -vE '$$(NGCC_EXCLUDE_RE)')
endif
SRCFILES_$(1) := $$(addprefix src/$(1)/,$$(SRCS_$(1)))
ifeq ($$(filter drng.c,$$(notdir $$(SRCS_$(1)))),)
ifneq ($$(NODRNG_$(1)),1)
SRCFILES_$(1) += $(API)/drng.c
endif
endif
ifneq ($(NGCC_TYPE),hash)
ifeq ($$(filter auxfunc.c,$$(notdir $$(SRCS_$(1)))),)
ifneq ($$(NOAUX_$(1)),1)
SRCFILES_$(1) += $(API)/auxfunc.c
endif
endif
endif
OBJS_$(1) := $$(patsubst %,build/$(1)/%.o,$$(subst /,__,$$(SRCFILES_$(1))))
HASCXX_$(1) := $$(filter %.cpp %.cc,$$(SRCFILES_$(1)))
LINKER_$(1) := $$(if $$(HASCXX_$(1)),$(CXX),$(CC))
INCFLAGS_$(1) := -Isrc/$(1) -I$(API) $$(INC_$(1))
SHIMDEFS_ALL_$(1) := $(NGCC_TYPE_DEF) -DNGCC_ID='"$(NGCC_ID)"' -DNGCC_ALG='"$(NGCC_ALG)"' \
    -DNGCC_VARIANT='"$$(if $$(findstring Optimi,$(2)),Optimized_Implementation,Reference_Implementation)"' \
    -DNGCC_SRCDIR='"$(2)"' -DNGCC_FLAGS='"$(NGCC_OPT) $$(CFLAGS_$(1))"' $$(SHIMDEFS_$(1))

build/$(1)/shim.o: $(API)/link_shim.c $(API)/link_common.h $(API)/link_$(NGCC_TYPE).h | build/$(1)
	$(CC) $(NGCC_CFLAGS) $$(CFLAGS_$(1)) $$(DEFS_$(1)) $$(INCFLAGS_$(1)) $$(SHIMDEFS_ALL_$(1)) -c -o $$@ $$<

# one rule per source file so paths are explicit
$$(foreach s,$$(SRCFILES_$(1)),$$(eval $$(call ngcc_obj_rule,$(1),$$(s))))

lib/lib$(1).so: build/$(1)/shim.o $$(OBJS_$(1)) | lib
	$$(LINKER_$(1)) $(NGCC_LDFLAGS) -o $$@ $$^ $(NGCC_LDLIBS) $$(LDLIBS_$(1))

build/$(1):
	mkdir -p $$@

test-$(1): lib/lib$(1).so $(HARNESS) | results kat
	@$(HARNESS) --out-dir kat --kat-dir '$$(if $$(KATDIR_$(1)),$$(KATDIR_$(1)),$(NGCC_KAT_DIR))' \
	    $$(if $$(wildcard $(NGCC_MANIFEST)),--kat-sha $(NGCC_MANIFEST)) --label $(1) \
	    $$(if $$(KATNAME_$(1)),--kat-name '$$(KATNAME_$(1))') $$(TESTFLAGS_$(1)) $(NGCC_TESTFLAGS) \
	    $$< > results/$(1).log 2>&1; rc=$$$$?; \
	  if ! tail -1 results/$(1).log | grep -q '^RESULT'; then \
	    echo "RESULT $(NGCC_ID) $(1) CRASH exit=$$$$rc" >> results/$(1).log; \
	    test $$$$rc -ne 0 || rc=7; \
	  fi; \
	  tail -1 results/$(1).log; exit $$$$rc
.PHONY: test-$(1)
TESTS += test-$(1)

# hash this instance's reference KAT files into the manifest
manifest-$(1): lib/lib$(1).so $(HARNESS)
	@$(HARNESS) --quiet --kat-dir '$$(if $$(KATDIR_$(1)),$$(KATDIR_$(1)),$(NGCC_KAT_DIR))' \
	    $$(if $$(KATNAME_$(1)),--kat-name '$$(KATNAME_$(1))') $$(TESTFLAGS_$(1)) --label $(1) --write-manifest $(NGCC_MANIFEST) $$<
.PHONY: manifest-$(1)
MANIFESTS += manifest-$(1)
endef

# $(1)=label $(2)=source path
define ngcc_obj_rule
build/$(1)/$$(subst /,__,$(2)).o: $(2) | build/$(1)
ifeq ($$(suffix $(2)),.c)
	$(CC) $(NGCC_CFLAGS) $$(CFLAGS_$(1)) $$(DEFS_$(1)) $$(INCFLAGS_$(1)) -c -o $$@ $$<
else ifeq ($$(suffix $(2)),.S)
	$(CC) $(NGCC_CFLAGS) $$(CFLAGS_$(1)) $$(DEFS_$(1)) $$(INCFLAGS_$(1)) -c -o $$@ $$<
else
	$(CXX) $(NGCC_CXXFLAGS) $$(CXXFLAGS_$(1)) $$(DEFS_$(1)) $$(INCFLAGS_$(1)) -c -o $$@ $$<
endif
endef

lib results kat:
	mkdir -p $@

$(HARNESS):
	$(MAKE) -C $(API) harness
