#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "laurus_768.h"

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
	fprintf(fp, "%s input %d bytes achieve %.01f Mbps£¬the clock cycle for one execution is %.0f\n", func_name, bytelen, SPEED, clc*1e9 / SPEED); 

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

void Laurus_768_SelfTest()
{
	int i;
	laurus_state state;
	uint8_t message[6400], h0[2000], h1[2000], h2[2000];

	randombytes(message, 6400);
	
	laurus_768(h0, message, 6400);
	laurus_768_init(&state);
	laurus_768_absorb(&state, message, 37);
	laurus_768_absorb(&state, message + 37, 640);
	laurus_768_absorb(&state, message + 677, 5723);
	laurus_768_finalize(&state);
	laurus_768_squeeze(h1, 96, &state);
	for (i = 0; i < 96; i++)
	{
		if (h0[i] != h1[i])
		{
			printf(" Laurus_768 test is error!\n");
		}
	}

	printf("\n Laurus_768 test is right!\n");
}

void Laurus_768_SpeedTest(FILE *fp)
{
	uint8_t hash[65536];
	int i, start = 24, value[8] = { 32,128,512,1024,4096,8192,16384,65536 };

	randombytes(hash, 65536);
	Environment(fp);

	for (i = 0; i < 8; i++)
	{
		SPEED_START(start - i);
		laurus_768(hash, hash, value[i]);
		SPEED_FINISH(fp, "laurus_768", value[i], 3.7);
	}
}

void main()
{
	srand((unsigned)time(NULL));

	Laurus_768_SelfTest();
	Laurus_768_SpeedTest(stdout);
}