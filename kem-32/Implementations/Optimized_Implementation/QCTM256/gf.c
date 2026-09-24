#include <stdio.h>
#include <stdlib.h>
#include "gf.h"
#include "rng.h"


static unsigned prim_poly[MAX_EXT_DEG + 1] = {
    01,		        
    03,		        
    07, 		      
    013, 		      
    023, 		      
    045, 		      
    0103, 		    
    0203, 		    
    0435, 		    
    01041, 		    
    02011,		    
    04005,		    
    010123,		    
    020033,		    
    042103,		    
    0100003,	    
    0210013,	    
    0400011,			
    01000201,			
    02000047,			
    04000011,			
    010000005,		
    020000003,		
    040000041,		
    0100000207,		
    0200000011,		
    0400000107,		
    01000000047,	
    02000000011,	
    04000000005,	
    010040000007,	
    020000000011	
};

static gfelt_t gf_mul_mod_poly(gfelt_t x, gfelt_t y)
{
    uint32_t a = x;
    uint32_t b = y;
    uint32_t r = 0;
    uint32_t top = UINT32_C(1) << gf_extd();
    uint32_t mask = (uint32_t)gf_ord();
    uint32_t modulus = prim_poly[gf_extd()];

    while (b != 0) {
        if ((b & 1U) != 0) {
            r ^= a;
        }
        b >>= 1;
        a <<= 1;
        if ((a & top) != 0) {
            a ^= modulus;
        }
        a &= mask;
    }

    return (gfelt_t)(r & mask);
}

static gfelt_t gf_pow_mod_poly(gfelt_t x, uint32_t e)
{
    gfelt_t r = 1;

    while (e != 0) {
        if ((e & 1U) != 0) {
            r = gf_mul_mod_poly(r, x);
        }
        e >>= 1;
        if (e != 0) {
            x = gf_mul_mod_poly(x, x);
        }
    }

    return r;
}

static int gf_is_primitive_generator(gfelt_t x)
{
    uint32_t order = (uint32_t)gf_ord();
    uint32_t remaining = order;
    uint32_t p;

    if (x == 0) {
        return 0;
    }

    for (p = 2; p <= remaining / p; p++) {
        if ((remaining % p) == 0) {
            if (gf_pow_mod_poly(x, order / p) == 1) {
                return 0;
            }
            while ((remaining % p) == 0) {
                remaining /= p;
            }
        }
    }
    if (remaining > 1 && gf_pow_mod_poly(x, order / remaining) == 1) {
        return 0;
    }

    return 1;
}

static gfelt_t gf_find_primitive_generator(void)
{
    uint32_t x;

    for (x = 2; x < (uint32_t)gf_card(); x++) {
        if (gf_is_primitive_generator((gfelt_t)x)) {
            return (gfelt_t)x;
        }
    }
    return 0;
}



void gf_init_exp() {
  int i;
  gfelt_t generator;

  generator = gf_find_primitive_generator();
  if (generator == 0) {
    fprintf(stderr,"failed to find primitive generator for GF(2^%d)!\n",
            gf_extd());
    exit(0);
  }
  gf_exp[0] = 1;
  for (i = 1; i < gf_ord(); ++i) {
    gf_exp[i] = gf_mul_mod_poly(gf_exp[i - 1], generator);
  }

  gf_exp[gf_ord()] = 1;
}


void gf_init_log() {
  int i;

  gf_log[0] = gf_ord();
  for (i = 0; i < gf_ord() ; ++i)
    gf_log[gf_exp[i]] = i;
}

int init_done = 0;

int gf_init(int extdeg) {
  if ((extdeg < MIN_EXT_DEG) || (extdeg > MAX_EXT_DEG)) {
    fprintf(stderr,"Extension degree %d not implemented !\n", extdeg);
    exit(0);
  }
  if (init_done != extdeg) {
    if (init_done) {
      free(gf_exp);
      free(gf_log);
    }
		gf_extension_degree = extdeg;
    gf_cardinality = 1 << extdeg;
    gf_multiplicative_order = gf_cardinality - 1;
		gf_log = (gfindex_t *) malloc((1 << gf_extd()) * sizeof (gfindex_t));
		if (gf_log == NULL) {
			fprintf(stderr,"allocation failed for gf_log!\n");
			return 0;
		}
		gf_exp = (gfelt_t *) malloc((1 << gf_extd()) * sizeof (gfelt_t));
		if (gf_exp == NULL) {
			fprintf(stderr,"allocation failed for gf_exp!\n");
			free(gf_log);
			return 0;
		}
    gf_init_exp();
    gf_init_log();
		init_done = extdeg;
  }

  return extdeg;
}

void gf_clear() {
	if (init_done) {
		free(gf_exp);
		free(gf_log);
	}
	init_done = 0;
}


void gf_pow(gf_t a, gf_t x, int i) {
	uint64_t y;

  if (i == 0)
    a[0] = 1;
  else if (x[0] == 0)
    a[0] = 0;
  else {

    while (i >> gf_extd())
      i = (i & (gf_ord())) + (i >> gf_extd());
    y = i * gf_log[x[0]];
    while (y >> gf_extd())
      y = (y & (gf_ord())) + (i >> gf_extd());
    a[0] = gf_exp[y];
  }
}

static int ucharToInt(unsigned char * c, int s) {
    int i, res = 0;
    
    for(i=0; i < s; i++){
        res <<= 8;
        res ^= (c[i] & 0xff);
    }
    
    return res;
}

void gf_set_rand(gf_t a) {

	int m = gf_extd();
	unsigned char * rand = malloc(((m - 1)/ 8 + 1) * sizeof(unsigned char));
	randombytes(rand, (m - 1)/ 8 + 1);
	
	a[0] = 0;
	while (m > 0) {
		a[0] = ucharToInt(rand + m/8, 1) ^ (a[0] << 8);
		m -= 8;
	}
	
	free(rand);
	a[0] &= gf_ord();
}
