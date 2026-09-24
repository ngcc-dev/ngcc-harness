#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "drng.h"
#include "cs.h"

#define Environment(fp) { time_t t; time(&t); \
fprintf(fp, "\n ========================================================================================================");\
fprintf(fp, "\n Test platform");\
fprintf(fp, "\n Compilation environment: Intel(R) Core(TM) i3-6100 CPU @3.70GHz, 16GB RAM, WIN7 64-bit system");\
fprintf(fp, "\n Compiler: VS2022, Release model, x64 solution platform, Program execution time: %s", ctime(&t));\
fprintf(fp, " ========================================================================================================");\
fprintf(fp, "\n Test AlgorithmInstance: %s\n", ALGORITHM_NAME); \
fprintf(fp, "\n PublicKey length: %d bytes, SecretKey length: %d bytes\n", PUBLICKEYBYTES, SECRETKEYBYTES); \
fprintf(fp, "\n Message length: %d bytes, SignatureText lenght: %d bytes", 64, SIGNATUREBYTES); \
fprintf(fp, "\n ========================================================================================================\n\n"); }


#define SPEED_START(N) {\
	unsigned int III, NNN, COUNT;\
	double START, FINISH, TOTALTIME, SPEED;\
	NNN = (N);\
	COUNT = 1 << NNN;\
	START = clock();\
	for(III = 0; III < COUNT; III++){

#define SPEED_PRINT(fp, func_name, clc) \
	if((stdout) != NULL){\
	fprintf(fp, "\n Test function: %s", func_name);\
	fprintf(fp, "\n Test count: 2^%d times", NNN);\
	fprintf(fp, "\n Test duration: %.3f sec", TOTALTIME);\
	fprintf(fp, "\n Test speed: %.01f/s", SPEED); }\
	fprintf(fp, "\n Test results: ");\
	fprintf(fp, " %s %s achieve %.01f/s, the clock cycle for one execution is %.0f\n", ALGORITHM_NAME, func_name, SPEED, clc*1e9 / SPEED); 

#define SPEED_FINISH(fp, func_name, clc) }\
	FINISH = clock();\
	TOTALTIME = (double)(FINISH - START)/CLOCKS_PER_SEC;\
	SPEED = (double)COUNT / TOTALTIME;\
	SPEED_PRINT(fp, func_name, clc);}


extern DRNG_ctx drng_algorithm;

#define TESTN (14 - lambda / 128)

void CS_SelfTest()
{
	int i, num = 1 << TESTN;
	uint8_t pk[PUBLICKEYBYTES], sk[SECRETKEYBYTES];
	uint8_t seed[64] = { 0 }, M[64], sig[SIGNATUREBYTES];

	init_random_number(&drng_algorithm, seed, 64);

	for (i = 0; i < num; i++)
	{
		CS_KeyGen(pk, sk);
		get_random_number(&drng_algorithm, M, 512);
		CS_Sign(sig, sk, M, 64);
		if (CS_Verify(pk, M, 64, sig) == false)
		{
			printf("\n CS test is error!\n");
			return;
		}
	}
	printf("\n All CS tests are right!\n\n");
}

void CS_SpeedTest(FILE* fp)
{
	uint8_t seed[64] = { 0 }, pk[PUBLICKEYBYTES], sk[SECRETKEYBYTES], sig[SIGNATUREBYTES];

	init_random_number(&drng_algorithm, seed, 64);

	Environment(fp);

	SPEED_START(TESTN + 4);
	CS_KeyGen(pk, sk);
	SPEED_FINISH(fp, "KeyGen", 3.7);


	SPEED_START(TESTN + 1);
	CS_Sign(sig, sk, pk, 64);
	SPEED_FINISH(fp, "Sign", 3.7);


	SPEED_START(TESTN + 4);
	CS_Verify(pk, pk, 64, sig);
	SPEED_FINISH(fp, "Verify", 3.7);
}

void main()
{
	CS_SelfTest();
	CS_SpeedTest(stdout);
}
