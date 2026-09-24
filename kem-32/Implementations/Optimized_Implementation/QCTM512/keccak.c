

void Keccak(unsigned int rate, unsigned int capacity, const unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen);

void FIPS202_SHAKE128(const unsigned char *input, unsigned int inputByteLen, unsigned char *output, int outputByteLen)
{
	Keccak(1344, 256, input, inputByteLen, 0x1F, output, outputByteLen);
}

void FIPS202_SHAKE256(const unsigned char *input, unsigned int inputByteLen, unsigned char *output, int outputByteLen)
{
	Keccak(1088, 512, input, inputByteLen, 0x1F, output, outputByteLen);
}

void FIPS202_SHA3_224(const unsigned char *input, unsigned int inputByteLen, unsigned char *output)
{
	Keccak(1152, 448, input, inputByteLen, 0x06, output, 28);
}

void FIPS202_SHA3_256(const unsigned char *input, unsigned int inputByteLen, unsigned char *output)
{
	Keccak(1088, 512, input, inputByteLen, 0x06, output, 32);
}

void FIPS202_SHA3_384(const unsigned char *input, unsigned int inputByteLen, unsigned char *output)
{
	Keccak(832, 768, input, inputByteLen, 0x06, output, 48);
}

void FIPS202_SHA3_512(const unsigned char *input, unsigned int inputByteLen, unsigned char *output)
{
	Keccak(576, 1024, input, inputByteLen, 0x06, output, 64);
}


typedef unsigned char UINT8;
typedef unsigned long long int UINT64;
typedef UINT64 tKeccakLane;

#ifndef LITTLE_ENDIAN
static UINT64 load64(const UINT8 *x)
{
	int i;
	UINT64 u=0;

	for(i=7; i>=0; --i) {
		u <<= 8;
		u |= x[i];
	}
	return u;
}

static void store64(UINT8 *x, UINT64 u)
{
	unsigned int i;

	for(i=0; i<8; ++i) {
		x[i] = u;
		u >>= 8;
	}
}

static void xor64(UINT8 *x, UINT64 u)
{
	unsigned int i;

	for(i=0; i<8; ++i) {
		x[i] ^= u;
		u >>= 8;
	}
}
#endif


#define ROL64(a, offset) ((((UINT64)a) << offset) ^ (((UINT64)a) >> (64-offset)))
#define i(x, y) ((x)+5*(y))

#ifdef LITTLE_ENDIAN
#define readLane(x, y)          (((tKeccakLane*)state)[i(x, y)])
#define writeLane(x, y, lane)   (((tKeccakLane*)state)[i(x, y)]) = (lane)
#define XORLane(x, y, lane)     (((tKeccakLane*)state)[i(x, y)]) ^= (lane)
#else
#define readLane(x, y)          load64((UINT8*)state+sizeof(tKeccakLane)*i(x, y))
#define writeLane(x, y, lane)   store64((UINT8*)state+sizeof(tKeccakLane)*i(x, y), lane)
#define XORLane(x, y, lane)     xor64((UINT8*)state+sizeof(tKeccakLane)*i(x, y), lane)
#endif

int LFSR86540(UINT8 *LFSR)
{
	int result = ((*LFSR) & 0x01) != 0;
	if (((*LFSR) & 0x80) != 0)
				(*LFSR) = ((*LFSR) << 1) ^ 0x71;
	else
		(*LFSR) <<= 1;
	return result;
}

void KeccakF1600_StatePermute(void *state)
{
	unsigned int round, x, y, j, t;
	UINT8 LFSRstate = 0x01;

	for(round=0; round<24; round++) {
		{   			tKeccakLane C[5], D;

						for(x=0; x<5; x++)
				C[x] = readLane(x, 0) ^ readLane(x, 1) ^ readLane(x, 2) ^ readLane(x, 3) ^ readLane(x, 4);
			for(x=0; x<5; x++) {
								D = C[(x+4)%5] ^ ROL64(C[(x+1)%5], 1);
								for (y=0; y<5; y++)
					XORLane(x, y, D);
			}
		}

		{   			tKeccakLane current, temp;
						x = 1; y = 0;
			current = readLane(x, y);
						for(t=0; t<24; t++) {
								unsigned int r = ((t+1)*(t+2)/2)%64;
								unsigned int Y = (2*x+3*y)%5; x = y; y = Y;
								temp = readLane(x, y);
				writeLane(x, y, ROL64(current, r));
				current = temp;
			}
		}

		{   			tKeccakLane temp[5];
			for(y=0; y<5; y++) {
								for(x=0; x<5; x++)
					temp[x] = readLane(x, y);
								for(x=0; x<5; x++)
					writeLane(x, y, temp[x] ^((~temp[(x+1)%5]) & temp[(x+2)%5]));
			}
		}

		{   			for(j=0; j<7; j++) {
				unsigned int bitPosition = (1<<j)-1; 				if (LFSR86540(&LFSRstate))
					XORLane(0, 0, (tKeccakLane)1<<bitPosition);
			}
		}
	}
}


#include <string.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void Keccak(unsigned int rate, unsigned int capacity, const unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen)
{
	UINT8 state[200];
	unsigned int rateInBytes = rate/8;
	unsigned int blockSize = 0;
	unsigned int i;

	if (((rate + capacity) != 1600) || ((rate % 8) != 0))
		return;

		memset(state, 0, sizeof(state));

		while(inputByteLen > 0) {
		blockSize = MIN(inputByteLen, rateInBytes);
		for(i=0; i<blockSize; i++)
			state[i] ^= input[i];
		input += blockSize;
		inputByteLen -= blockSize;

		if (blockSize == rateInBytes) {
			KeccakF1600_StatePermute(state);
			blockSize = 0;
		}
	}

			state[blockSize] ^= delimitedSuffix;
		if (((delimitedSuffix & 0x80) != 0) && (blockSize == (rateInBytes-1)))
		KeccakF1600_StatePermute(state);
		state[rateInBytes-1] ^= 0x80;
		KeccakF1600_StatePermute(state);

		while(outputByteLen > 0) {
		blockSize = MIN(outputByteLen, rateInBytes);
		memcpy(output, state, blockSize);
		output += blockSize;
		outputByteLen -= blockSize;

		if (outputByteLen > 0)
			KeccakF1600_StatePermute(state);
	}
}
