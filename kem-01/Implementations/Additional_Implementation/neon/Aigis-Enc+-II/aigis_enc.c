#include "api.h"
#include "poly.h"
#include "polyvec.h"
#include "cpucycles.h"
#include <stdlib.h>
#include <stdio.h>
#include "speed.h"

#define NTESTS 10000

int main()
{
  unsigned char pk[PK_BYTES];
  unsigned char sk[SK_BYTES];
  unsigned char ct[CT_BYTES];

  unsigned char ss[KEM_BYTES], ss1[KEM_BYTES];
  unsigned long long t0[NTESTS], t1[NTESTS], t2[NTESTS];

  int fail=0;

  printf("\n**********************************\n");
  printf("PK Sizes : %d\n",KEM_PUBLICKEYBYTES);
  printf("SK Sizes : %d\n",KEM_SECRETKEYBYTES);
  printf("CT Sizes: %d\n",KEM_CIPHERTEXTBYTES); 
  printf("**********************************\n\n");


  int i,j;

  for(i=0; i<NTESTS; i++)
  {
    t0[i] = cpucycles();
    kem_keygen(pk, sk);
	t0[i] = cpucycles() - t0[i];

	t1[i] = cpucycles();
	kem_enc(pk, ss, ct);
	t1[i] = cpucycles() - t1[i];

	t2[i] = cpucycles();
	fail = kem_dec(sk, ct, ss1);
	t2[i] = cpucycles() - t2[i];

	if (fail)
		printf("dec error %d!\n\n", i);
	for (j = 0; j < KEM_BYTES;j++)
		if (ss[j]!=ss1[j])
			printf("dec error %d!\n\n", i);

  }
  print_results("kem_keygen: ", t0, NTESTS);
  print_results("kem_enc:   ", t1, NTESTS);
  print_results("kem_dec:  ", t2, NTESTS);
  return 0;
}
