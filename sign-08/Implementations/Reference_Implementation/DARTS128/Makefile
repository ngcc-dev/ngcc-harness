CC ?= /usr/bin/cc
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -O3 -march=native -mtune=native -std=gnu99 \
  -fcommon

# ---------- source files ----------
SIGN_SRC = sign.c packing.c polyvec.c poly.c ntt.c reduce.c \
           sampler.c encoding.c polyfix.c polymat.c fft.c fixpoint.c

SYM_SRC  = symmetric.c

ICCS_SRC = drng.c auxfunc.c

KAT_SRC  = SIG_AlgorithmInstance.c

SOURCES  = $(SIGN_SRC) $(SYM_SRC) $(ICCS_SRC) $(KAT_SRC)

# ---------- test directory ----------
TEST_DIR = test

# ---------- targets ----------
.PHONY: all test KAT clean

BIN_DIR = bin

all: $(BIN_DIR)/KAT_SIG $(BIN_DIR)/test_mul $(BIN_DIR)/test_DARTS $(BIN_DIR)/test_speed

# ===================== make KAT =====================
KAT: $(BIN_DIR)/KAT_SIG

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/KAT_SIG: KAT_SIG.c $(SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ KAT_SIG.c $(SOURCES) -lm

# ===================== make test =====================
test: $(BIN_DIR)/test_mul $(BIN_DIR)/test_DARTS $(BIN_DIR)/test_speed

$(BIN_DIR)/test_mul: $(TEST_DIR)/test_mul.c $(SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_mul.c $(SOURCES) -lm

$(BIN_DIR)/test_DARTS: $(TEST_DIR)/test_DARTS.c $(SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_DARTS.c $(SOURCES) -lm

$(BIN_DIR)/test_speed: $(TEST_DIR)/test_speed.c $(TEST_DIR)/cpucycles.c $(TEST_DIR)/speed_print.c $(SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_speed.c $(TEST_DIR)/cpucycles.c $(TEST_DIR)/speed_print.c $(SOURCES) -lm

# ===================== clean =====================
clean:
	-rm -rf $(BIN_DIR)
	-rm -rf output