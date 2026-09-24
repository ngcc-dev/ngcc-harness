#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "laurus_xof.h"

#define Environment(fp) { time_t t; time(&t); \
fprintf(fp, "\n ========================================================================================================");\
fprintf(fp, "\n Test platform");\
fprintf(fp, "\n Compilation environment: Intel(R) Core(TM) i3-6100 CPU @3.70GHz, 16GB RAM, WIN7 64-bit system");\
fprintf(fp, "\n Compiler: VS2022, Release model, x64 solution platform, Program execution time: %s", ctime(&t));\
fprintf(fp, " ========================================================================================================\n\n");}


#define SPEED_START(N) {\
	unsigned int III, NNN, COUNT, TOTAL_BYTELEN;\
	double START, FINISH, TOTALTIME, SPEED;\
	NNN = (N);\
	COUNT = 1 << NNN;\
	START = clock();\
	for(III = 0; III < COUNT; III++){

#define SPEED_PRINT(fp, func_name, bytelen, clc) \
	TOTAL_BYTELEN = (bytelen);\
	if((stdout) != NULL){\
	fprintf(fp, "\n Test function: %s", func_name);\
	fprintf(fp, "\n Test count: 2^%d times", NNN);\
	fprintf(fp, "\n Test duration: %.3f sec", TOTALTIME);\
	fprintf(fp, "\n Test speed: %.01f Mbps", SPEED); }\
	fprintf(fp, "\n Test results: ");\
	fprintf(fp, "%s input %d bytes achieve %.01f Mbps, the clock cycle for one execution is %.0f\n", func_name, bytelen, SPEED, clc*1e9 / SPEED); 

#define SPEED_FINISH(fp, func_name, bytelen, clc) }\
	FINISH = clock();\
	TOTALTIME = (double)(FINISH - START)/CLOCKS_PER_SEC;\
	SPEED = (double)COUNT / TOTALTIME;\
	SPEED = SPEED * (bytelen) * 8.0 / 1024.0 / 1024.0;\
	SPEED_PRINT(fp, func_name, bytelen, clc);}


void randombytes(uint8_t* r, uint64_t len)
{
	uint64_t i;

	for (i = 0; i < len; i++)
	{
		r[i] = rand() & 0xFF;
	}
}

void Laurus_xof_SelfTest()
{
	int i;
	laurus_state state;
	uint8_t message[6400], h0[2000], h1[2000], h2[2000];

	randombytes(message, 6400);
	
    laurus_xof(h0, 1024, message, 6400);
	laurus_xof_init(&state);
	laurus_xof_absorb(&state, message, 37);
	laurus_xof_absorb(&state, message + 37, 640);
	laurus_xof_absorb(&state, message + 677, 5723);
	laurus_xof_finalize(&state);
	laurus_xof_squeeze(h1, 100, &state);
	laurus_xof_squeeze(h1 + 100, 337, &state);
	laurus_xof_squeeze(h1 + 437, 587, &state);
	laurus_xof_init(&state);
	laurus_xof_absorb(&state, message, 6400);
	laurus_xof_finalize(&state);
	laurus_xof_squeezeblocks(h2, 8, &state);
	for (i = 0; i < 1000; i++)
	{
		if ((h0[i] != h1[i]) || (h0[i] != h2[i]))
		{
			printf(" Laurus_xof is error!\n");
		}
	}

	printf("\n Laurus_xof test is right!\n");
}

void main()
{
	srand((unsigned)time(NULL));

	Laurus_xof_SelfTest();
}