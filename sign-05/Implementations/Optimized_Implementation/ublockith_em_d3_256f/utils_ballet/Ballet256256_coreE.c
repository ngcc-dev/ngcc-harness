#include "Ballet256256_coreE.h"


void BalletGenRK_256_256_ENC(u8i * Rk, u8i * K)
{
	u64i i;
	u64i  k0, k1, k2, k3;
	u64i *p = (u64i *)Rk;
	k1 = *((u64i *)K);
	k0 = *((u64i *)(K + 8));
	k3 = *((u64i *)(K + 16));
	k2 = *((u64i *)(K + 24));

	k0 = SWAP64(k0);
	k1 = SWAP64(k1);
	k2 = SWAP64(k2);
	k3 = SWAP64(k3);
	 
		for (i = 0; i < RoundBallet256256; i += 2)
		{
			*p = k0;
			*(p + 1) = k1;
			//   printf("%016I64x %016I64x\n", k00, k01 ); 
			k0 ^= ((k2 << 3) | (k3 >> 61)) ^ ((k2 << 5) | (k3 >> 59)) ^ i;
			k1 ^= ((k3 << 3) | (k2 >> 61)) ^ ((k3 << 5) | (k2 >> 59));
			*(p + 2) = k2;
			*(p + 3) = k3;
			//printf("%016I64x %016I64x\n", k10, k11 ); 
			k2 ^= ((k0 << 3) | (k1 >> 61)) ^ ((k0 << 5) | (k1 >> 59)) ^ (i + 1);
			k3 ^= ((k1 << 3) | (k0 >> 61)) ^ ((k1 << 5) | (k0 >> 59));
			p += 4;
		}

 
} 

void Ballet256256EncDataS(unsigned char *out, unsigned char *pt, unsigned char *rk)
{
	int i;
	u64i t0;
	u64i *trk = (u64i *)rk;
	u64i a = *((u64i *)pt);
	u64i b = *((u64i *)(pt + 8));
	u64i c = *((u64i *)(pt + 16));
	u64i d = *((u64i *)(pt + 24));

	a = SWAP64(a); b = SWAP64(b);
	c = SWAP64(c); d = SWAP64(d);

	for (i = 0; i < RoundBallet256256; i += 2)
	{
		t0 = b ^ c;
		b ^= (*(trk + 1));
		c ^= (*trk);
		b = ROTL64(b, 6);
		d = ROTL64(d, 15);
		a = ROTL64(a, 6);
		a += ROTL64(t0, 9);
		d += ROTL64(t0, 14);
		t0 = a ^ d;
		b += ROTL64(t0, 9);
		d ^= (*(trk + 2));
		a ^= (*(trk + 3));
		c = ROTL64(c, 15);
		c += ROTL64(t0, 14);
		trk += 4;

	}
	a = SWAP64(a); b = SWAP64(b);
	c = SWAP64(c); d = SWAP64(d);
	*((u64i *)out) = b;
	*((u64i *)(out + 8)) = a;
	*((u64i *)(out + 16)) = d;
	*((u64i *)(out + 24)) = c;
}
