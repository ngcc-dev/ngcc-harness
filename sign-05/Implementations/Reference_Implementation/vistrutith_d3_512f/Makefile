CC ?= gcc

# Reference implementation: portable C only, no instruction-set acceleration sources or flags.
CFLAGS = -O2 -std=c11 -pedantic -Wall -Wextra -I. -I./utils_vistrutah -include fallbacks.h -DSBOX_TABLE
LDFLAGS = -lm

NDEBUG ?= 1
ifeq ($(NDEBUG),1)
CFLAGS += -DNDEBUG -O3
endif

SOURCES = auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c universal_hashing.c utils.c utils_vistrutah/sig_impl_vistrutith.c utils_vistrutah/vistrutah.c utils_vistrutah/vistrutith_constraints.c utils_vistrutah/vistrutith_vistrutah_512.c utils_vistrutah/vistrutith_witness.c vistrutith_d3_512f.c vistrutith_d3_512f_AlgorithmInstance.c vole.c 
OBJECTS = $(SOURCES:.c=.o)

all: vistrutith_d3_512f_bench

KAT_SIG: KAT_SIG.c $(SOURCES)
	$(CC) $(CFLAGS) -o $@ KAT_SIG.c $(SOURCES) $(LDFLAGS)

vistrutith_d3_512f_bench: vistrutith_d3_512f_bench.c $(SOURCES)
	$(CC) $(CFLAGS) -o $@ vistrutith_d3_512f_bench.c $(SOURCES) $(LDFLAGS)

clean:
	rm -f vistrutith_d3_512f_bench vistrutith_d3_512f_test KAT_SIG $(OBJECTS)
	rm -f *.o */*.o
	rm -rf output

.PHONY: all clean KAT_SIG vistrutith_d3_512f_bench
