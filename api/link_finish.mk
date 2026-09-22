# NGCC uniform build rules, second half. Include at the END of a candidate
# Makefile, after all $(eval $(call ngcc_instance,...)) lines.

NGCC_TEST_INSTANCES ?= $(INSTANCES)

libs: $(LIBS)

test: test-prep
	@rc=0; $(MAKE) --no-print-directory -k $(TESTS) || rc=$$?; \
	  cat $(addprefix results/,$(addsuffix .log,$(NGCC_TEST_INSTANCES))) | grep '^RESULT' > results/summary.tsv; \
	  echo "$(NGCC_ID): $$(grep -c ' PASS' results/summary.tsv) pass, $$(grep -vc ' PASS' results/summary.tsv) fail (results/summary.tsv)"; \
	  exit $$rc

$(TESTS): | test-prep
test-prep: | results
	@rm -f results/summary.tsv

# manifest targets run sequentially: they all rewrite $(NGCC_MANIFEST)
manifest: libs
	@for t in $(MANIFESTS); do $(MAKE) --no-print-directory $$t || echo "  ($$t: no reference KAT, skipped)"; done; \
	  echo "$(NGCC_ID): $$(wc -l < $(NGCC_MANIFEST)) KAT digests in $(NGCC_MANIFEST)"

list:
	@true $(foreach i,$(INSTANCES),; echo "$(i)  ->  $(SRCDIR_$(i))")

clean:
	rm -rf build lib src results kat

.PHONY: libs test test-prep manifest list clean
