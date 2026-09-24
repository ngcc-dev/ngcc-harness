
#include "symmetric.h"
#include "auxfunc.h"

void hash_f(uint8_t *buf, const uint8_t *msg)
{
	uint8_t data[1 + NTRUOAEP_PUBLICKEYBYTES] = {0x0};

	for (int i = 0; i < NTRUOAEP_PUBLICKEYBYTES; i++)
	{
		data[i+1] = msg[i];
	}

	pseudohash(512, data, (NTRUOAEP_PUBLICKEYBYTES + 1)*8, buf);
}

void hash_g(uint8_t *buf, const uint8_t *msg)
{
	uint8_t data[1 + NTRUOAEP_N / 4] = {0x1};

	for (int i = 0; i < NTRUOAEP_N / 4; i++)
	{
		data[i+1] = msg[i];
	}

	pseudoXOF(NTRUOAEP_N / 4 * 8, data, (NTRUOAEP_N / 4 + 1)*8, buf);
}


void hash_h_prime(uint8_t *buf, const uint8_t *msg)
{
	uint8_t data[1 + NTRUOAEP_N / 2 + NTRUOAEP_SYMBYTES] = {0x5};

	for (int i = 0; i < NTRUOAEP_N / 2 + NTRUOAEP_SYMBYTES; i++)
	{
		data[i+1] = msg[i];
	}

	pseudoXOF(KeyConfirmation_BYTES * 8, data, (1 + NTRUOAEP_N / 2 + NTRUOAEP_SYMBYTES) * 8, buf);
}
