#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "randombytes.h"
#include "dtru.h"
#include "params.h"
#include "speed.h"

#define NTESTS 10000
uint64_t t[NTESTS];

void test_pke()
{
  unsigned int i;
  unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES];
  unsigned char m[DTRU_MSGBYTES],  m2[DTRU_MSGBYTES];
  unsigned char coins[DTRU_COINBYTES_KEYGEN + DTRU_COINBYTES_ENC];
  unsigned char pk[DTRU_PKE_PUBLICKEYBYTES], sk[DTRU_PKE_SECRETKEYBYTES];
  randombytes(m, DTRU_MSGBYTES);

  for (i = 0; i < NTESTS; i++)
  {
    do 
    {
        randombytes(coins, DTRU_COINBYTES_KEYGEN + DTRU_COINBYTES_ENC);
    } while (pke_keygen(pk, sk, coins));

    pke_enc(ct, pk, m, coins + DTRU_COINBYTES_KEYGEN);
    pke_dec(m2, ct, sk);
    for(int j = 0; j < DTRU_MSGBYTES; j ++)
    {
        if(m[j]!=m2[j])
        {
            printf("error key %d, m = %d, m2 = %d\n", j, m[j], m2[j]);
            i = NTESTS + 1;
            break;
        }
    }
  }

  if (i == NTESTS)
      printf("pke test pass\n\n");
  else
      printf("pke test fail\n\n");

  for (i = 0; i < NTESTS; i++)
  {
      t[i] = cpucycles();
      pke_keygen(pk, sk, coins);
  }
  print_results("dtru_pke_keygen: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
      t[i] = cpucycles();
      pke_enc(ct, pk, m, coins + DTRU_COINBYTES_KEYGEN);
  }
  print_results("dtru_pke_enc: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
      t[i] = cpucycles();
      pke_dec(m2, ct, sk);
  }
  print_results("dtru_pke_dec: ", t, NTESTS);

}

int main()
{
  test_pke();
  return 0;
}
