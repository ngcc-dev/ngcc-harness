# NGCC Round 1 reproduction harness. See README.md, api/README.md, tools/README.md.
#
#   make -j8             build harness, reproducer and every included candidate
#   make -j8 test        run the KAT harness on every library, summary in results/
#   make -C kem-01       one candidate (make -C kem-01 test: its KATs)
#   make status          re-aggregate results/ without re-running anything
#   make reproduce       tools/reproduce.sh
#   make check-vulnerabilities  validate stable issue IDs and checker coverage
#   make check-reference-data   validate all specs and extracted parameters
#   make manifest        (re)compute kat.sha256 manifests (needs the Test_Vectors present)
#   make clean           remove all build outputs, libraries, results, harness

NGCC1 ?= ../ngcc1
CANDIDATES := $(sort $(patsubst %/Makefile,%,$(wildcard sign-*/Makefile kem-*/Makefile kex-*/Makefile hash-*/Makefile)))

.PHONY: all harness tools exploits test status reproduce design-audit check-vulnerabilities check-reference-data sync-reference-data manifest clean $(CANDIDATES) \
        $(addprefix test-,$(CANDIDATES)) $(addprefix manifest-,$(CANDIDATES)) $(addprefix clean-,$(CANDIDATES))

all: harness tools $(CANDIDATES) exploits

harness:
	$(MAKE) -C api harness

tools:
	$(MAKE) -C tools

$(CANDIDATES): harness | results
	@$(MAKE) --no-print-directory -C $@ libs > results/build-$@.log 2>&1 && echo "BUILD $@ ok" || { echo "BUILD $@ FAILED (results/build-$@.log)"; }

exploits: sign-03 sign-34 kex-02 kex-05
	@$(MAKE) --no-print-directory -C sign-03 exploit
	@$(MAKE) --no-print-directory -C sign-34 exploit
	@$(MAKE) --no-print-directory -C kex-02 exploit
	@$(MAKE) --no-print-directory -C kex-05 replay exploit

test: $(addprefix test-,$(CANDIDATES))
	@$(MAKE) --no-print-directory status

$(addprefix test-,$(CANDIDATES)): test-%: % | results
	@$(MAKE) --no-print-directory -C $* test > results/test-$*.log 2>&1; tail -1 results/test-$*.log

results:
	mkdir -p results

status: | results
	@cat $(wildcard $(addsuffix /results/summary.tsv,$(CANDIDATES))) > results/summary.tsv 2>/dev/null; \
	 echo "candidates with Makefile: $(words $(CANDIDATES))"; \
	 echo "instances tested: $$(wc -l < results/summary.tsv)"; \
	 awk '{print $$4}' results/summary.tsv | sort | uniq -c | sort -rn

reproduce: tools
	tools/reproduce.sh

design-audit:
	python3 security/design_parameter_audit.py

check-vulnerabilities:
	python3 security/check_vulnerability_ids.py

check-reference-data:
	python3 tools/sync_reference_data.py --check

sync-reference-data:
	python3 tools/sync_reference_data.py $(NGCC1)

manifest: $(addprefix manifest-,$(CANDIDATES))
$(addprefix manifest-,$(CANDIDATES)): manifest-%: %
	@$(MAKE) --no-print-directory -C $* manifest 2>&1 | tail -1

clean: $(addprefix clean-,$(CANDIDATES))
	$(MAKE) -C tools clean
	rm -rf results bin
$(addprefix clean-,$(CANDIDATES)): clean-%:
	@$(MAKE) --no-print-directory -C $* clean
