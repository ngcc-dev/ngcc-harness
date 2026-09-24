/*Repeat is : 10000
Average times key_pair:          521731 
Average times enc:       549403 
Average times dec:       556539*/

/*Repeat is : 10000
Average times key_pair:          521552 
Average times enc:       549519 
Average times dec:       556520 */

#include "params.h"
#include "indcpa.h"
#include "KEM_scabbard128.h"
#include "api.h"
#include "poly.h"
//#include "randombytes.h"
#include "drng.h"

#include "cpucycles.c"
#include "verify.h"

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<time.h>
#include<immintrin.h>
#include<string.h>
#include <ctype.h>
#include <errno.h>


#define SEED_LEN_BYTES 64
#define KAT_KEM_SUCCESS 0
#define KAT_ALGORITHM_INSTANCE_NAME_INVALID -1
#define KAT_FILE_OPERATE_FAILED -2
#define KAT_KEM_CRYPTO_FAILURE -3
#define KAT_MEMORY_ALLOCATION_FAILED -4
#define KAT_KEM_SS_UNEQUAL -5


DRNG_ctx drng_algorithm;

int test_kem_cca()
{


  uint8_t pk[SCABBARD_PUBLICKEYBYTES];
  uint8_t sk[SCABBARD_SECRETKEYBYTES];
  uint8_t c[SCABBARD_CIPHERTEXTBYTES];	
  uint8_t k_a[SCABBARD_SSBYTES], k_b[SCABBARD_SSBYTES];
	
  unsigned char *nonce;
  DRNG_ctx drng_seed;
  unsigned char *seed;
	
  uint64_t i, j, repeat;
  repeat=1000;
  
  uint64_t CLOCK1,CLOCK2;
  uint64_t CLOCK_kp,CLOCK_enc,CLOCK_dec;

  	CLOCK1 = 0;
        CLOCK2 = 0;
	CLOCK_kp = CLOCK_enc = CLOCK_dec = 0;

	unsigned long long pk_len_bytes, sk_len_bytes, ss_len_bytes, ct_len_bytes;
	pk_len_bytes = kem_get_pk_len_bytes();
	sk_len_bytes = kem_get_sk_len_bytes();
	ss_len_bytes = kem_get_ss_len_bytes();
	ct_len_bytes = kem_get_ct_len_bytes();

	time_t t;
   	nonce = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
	for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
	{
		memcpy(nonce + 4 * i, "seed", 4);
	}
	init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

  	for(i=0; i<repeat; i++)
  	{
	    CLOCK1=cpucycles();	
	    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
	    CLOCK2=cpucycles();	
	    CLOCK_kp=CLOCK_kp+(CLOCK2-CLOCK1);	

	    CLOCK1=cpucycles();
	    kem_enc(pk, pk_len_bytes, k_a, &ss_len_bytes, c, &ct_len_bytes);
	    CLOCK2=cpucycles();	
	    CLOCK_enc=CLOCK_enc+(CLOCK2-CLOCK1);	

	    CLOCK1=cpucycles();
	    kem_dec(sk, sk_len_bytes, c, ct_len_bytes, k_b, &ss_len_bytes);
	    CLOCK2=cpucycles();	
	    CLOCK_dec=CLOCK_dec+(CLOCK2-CLOCK1);	
  
		for(j=0; j<SCABBARD_SSBYTES; j++){
			if(k_a[j] != k_b[j]){
				printf("Repeat:%ld\t j : %lu \t %u \t %u\n", i, j, k_a[j], k_b[j]);
				printf("----- ERR CCA KEM ------\n");
				return 0;		
				break;
			}
	    }
   		
  	}
	
	printf("Repeat is : %ld\n",repeat);
	printf("Average times key_pair: \t %lu \n",CLOCK_kp/repeat);
	printf("Average times enc: \t %lu \n",CLOCK_enc/repeat);
	printf("Average times dec: \t %lu \n",CLOCK_dec/repeat);
	
	return 0;
}

int main()
{
	test_kem_cca();
	return 0;
}
