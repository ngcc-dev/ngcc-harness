CC ?= gcc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic
X86_REF_FLAGS := -std=c99 -Wpedantic -Wall -Wextra -O2 -fPIC
X86_OPT_FLAGS := -O3 -march=x86-64 -mavx2 -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra -fPIC
X86_OPT_LINK := -shared -Wl,--version-script=../api/link_exports.map -Wl,-Bsymbolic -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -flto

ZCDMC_LEVELS := 1280-512 1280-768 1280-1024 1536-512 1536-768 1536-1024

.PHONY: all clean build-mithril-ref build-mithril-avx2 build-scloud-ref build-scloud-avx2 \
        build-zcdmc-ref build-zcdmc-avx2 build-bit-ref build-bit-avx2 \
        smoke-x86 smoke-mithril-avx2 smoke-scloud-avx2 smoke-zcdmc-avx2 \
        smoke-bit-avx2 check
all: ngcc_perf

build-mithril-ref:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../kem-22 NGCC_CFLAGS='$(X86_REF_FLAGS)' \
		$(addprefix lib/libMithril-,$(addsuffix .so,128 256 512))
	$(MAKE) -C ../kem-22 $(addprefix test-Mithril-,128 256 512)

build-mithril-avx2:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../kem-22 PERF_OPT=1 NGCC_CFLAGS='$(X86_OPT_FLAGS)' NGCC_LDFLAGS='$(X86_OPT_LINK)' \
		$(addprefix lib/libMithril-,$(addsuffix -avx2.so,128 256 512))
	$(MAKE) -C ../kem-22 PERF_OPT=1 $(addprefix test-Mithril-,$(addsuffix -avx2,128 256 512))

build-scloud-ref:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../kem-35 -f ../performance/kem-35.mk NGCC_CFLAGS='$(X86_REF_FLAGS)' lib/libScloudplus-128-SHAKE-ref.so
	$(MAKE) -C ../kem-35 -f ../performance/kem-35.mk test-Scloudplus-128-SHAKE-ref

build-scloud-avx2:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../kem-35 -f ../performance/kem-35.mk PERF_AVX2=1 \
		NGCC_CFLAGS='$(X86_OPT_FLAGS)' NGCC_LDFLAGS='$(X86_OPT_LINK)' lib/libScloudplus-128-SHAKE-avx2.so
	$(MAKE) -C ../kem-35 -f ../performance/kem-35.mk PERF_AVX2=1 test-Scloudplus-128-SHAKE-avx2

build-zcdmc-ref:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../hash-31 NGCC_CFLAGS='$(X86_REF_FLAGS)' \
		$(addprefix lib/libZC-DMC-,$(addsuffix .so,$(ZCDMC_LEVELS)))
	$(MAKE) -C ../hash-31 $(addprefix test-ZC-DMC-,$(ZCDMC_LEVELS))

build-zcdmc-avx2:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../hash-31 PERF_OPT=1 NGCC_CFLAGS='$(X86_OPT_FLAGS)' NGCC_LDFLAGS='$(X86_OPT_LINK)' \
		$(addprefix lib/libZC-DMC-,$(addsuffix -avx2.so,$(ZCDMC_LEVELS)))
	$(MAKE) -C ../hash-31 PERF_OPT=1 $(addprefix test-ZC-DMC-,$(addsuffix -avx2,$(ZCDMC_LEVELS)))

build-bit-ref:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../sign-02 NGCC_CFLAGS='$(X86_REF_FLAGS)' \
		$(addprefix lib/libBiT-,$(addsuffix .so,128 256 512))
	$(MAKE) -C ../sign-02 $(addprefix test-BiT-,128 256 512)

build-bit-avx2:
	$(MAKE) -C ../api harness
	$(MAKE) -B -C ../sign-02 PERF_OPT=1 NGCC_CFLAGS='$(X86_OPT_FLAGS)' NGCC_LDFLAGS='$(X86_OPT_LINK)' \
		$(addprefix lib/libBiT-,$(addsuffix -avx2.so,128 256 512))
	$(MAKE) -C ../sign-02 PERF_OPT=1 $(addprefix test-BiT-,$(addsuffix -avx2,128 256 512))

smoke-mithril-avx2: build-mithril-avx2 ngcc_perf
	@for level in 128 256 512; do \
		python3 smoke.py --library ../kem-22/lib/libMithril-$$level-avx2.so \
			--kat-log ../kem-22/results/Mithril-$$level-avx2.log || exit 1; \
	done

smoke-scloud-avx2: build-scloud-avx2 ngcc_perf
	@python3 smoke.py --library ../kem-35/lib/libScloudplus-128-SHAKE-avx2.so \
		--kat-log ../kem-35/results/Scloudplus-128-SHAKE-avx2.log

smoke-zcdmc-avx2: build-zcdmc-avx2 ngcc_perf
	@for level in $(ZCDMC_LEVELS); do \
		python3 smoke.py --library ../hash-31/lib/libZC-DMC-$$level-avx2.so \
			--kat-log ../hash-31/results/ZC-DMC-$$level-avx2.log || exit 1; \
	done

smoke-bit-avx2: build-bit-avx2 ngcc_perf
	@for level in 128 256 512; do \
		python3 smoke.py --library ../sign-02/lib/libBiT-$$level-avx2.so \
			--kat-log ../sign-02/results/BiT-$$level-avx2.log || exit 1; \
	done

# Sequential sub-makes avoid racing on the shared API harness under make -j.
smoke-x86:
	$(MAKE) smoke-mithril-avx2
	$(MAKE) smoke-scloud-avx2
	$(MAKE) smoke-zcdmc-avx2
	$(MAKE) smoke-bit-avx2

check:
	python3 validate.py

ngcc_perf: ngcc_perf.c ../api/link_common.h ../api/link_kem.h ../api/link_sig.h ../api/link_hash.h
	$(CC) $(CFLAGS) -o $@ ngcc_perf.c -ldl

clean:
	rm -f ngcc_perf
