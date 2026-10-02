CC ?= gcc

# Reference implementation: portable C only, no instruction-set acceleration sources or flags.
CFLAGS = -O2 -std=c11 -pedantic -Wall -Wextra -I. -I./utils_ballet -I./utils_ublock -include fallbacks.h
CFLAGS += -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
LDFLAGS = -lm

NDEBUG ?= 1
ifeq ($(NDEBUG),1)
CFLAGS += -DNDEBUG -O3
endif

SOURCES = auxfunc.c bavc.c compat.c drng.c fields.c owf.c params.c prg.c random_oracle.c sig_impl.c ublockith_d3_256f.c ublockith_d3_256f_AlgorithmInstance.c universal_hashing.c utils.c utils_ballet/Ballet256256_coreE.c utils_ublock/sig_impl_ublock.c utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c utils_ublock/ublock_core.c utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c utils_ublock/ublockith_ublock_256.c vole.c 
OBJECTS = $(SOURCES:.c=.o)

all: ublockith_d3_256f_bench

KAT_SIG: KAT_SIG.c $(SOURCES)
	$(CC) $(CFLAGS) -o $@ KAT_SIG.c $(SOURCES) $(LDFLAGS)

ublockith_d3_256f_bench: ublockith_d3_256f_bench.c $(SOURCES)
	$(CC) $(CFLAGS) -o $@ ublockith_d3_256f_bench.c $(SOURCES) $(LDFLAGS)

clean:
	rm -f ublockith_d3_256f_bench ublockith_d3_256f_test KAT_SIG $(OBJECTS)
	rm -f *.o */*.o
	rm -rf output

.PHONY: all clean KAT_SIG ublockith_d3_256f_bench
