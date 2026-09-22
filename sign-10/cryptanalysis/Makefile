# sign-10 (Facto-DSA) cryptanalysis.
#
# NOTE: unlike the other candidate Makefiles in this repository, this one does
# NOT build the submitter's reference implementation -- no sign-10 source is
# included here.  It builds a self-contained reimplementation of Facto-DSA
# written from the specification (Algorithms 1-11, q = 65519), plus the attack.
# See ../README.md for what is demonstrated and what is not.

CC      ?= gcc
CFLAGS  ?= -O2 -march=native -Wall -Wno-misleading-indentation -Wno-unused-result
LDLIBS  ?= -lm
BIN      = build
TARGETS  = $(BIN)/selftest $(BIN)/attack1 $(BIN)/attack3

.PHONY: all clean test
all: $(TARGETS)

$(BIN):
	@mkdir -p $(BIN)

$(BIN)/%: src/%.c | $(BIN)
	$(CC) $(CFLAGS) -Isrc -o $@ $< $(LDLIBS)

# Reproduce the headline results.  Arguments are: n m seed max_degree
test: all
	@echo "== spec-faithful Facto-DSA at the three submitted parameter sets =="
	$(BIN)/selftest 10 13 1
	$(BIN)/selftest 17 32 1
	$(BIN)/selftest 32 62 1
	@echo
	@echo "== K2 = ker L2 recovered from the public key alone =="
	$(BIN)/attack1 5 8 1 10
	$(BIN)/attack1 6 10 1 10
	@echo
	@echo "== full chain to an equivalent triangular trapdoor =="
	$(BIN)/attack3 6 10 1 12

clean:
	rm -rf $(BIN)
