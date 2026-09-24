/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpucycles.h"

uint64_t cpucycles_overhead(void)
{
    uint64_t t0, t1, overhead = -1LL;
    unsigned int i;

    for(i=0;i<100000;i++) 
    {
        t0 = cpucycles();
        __asm__ volatile ("");
        t1 = cpucycles();
        if(t1 - t0 < overhead)
        overhead = t1 - t0;
    }

  return overhead;
}

static int cmp_uint64(const void *a, const void *b) 
{
    if(*(uint64_t *)a < *(uint64_t *)b) return -1;
    if(*(uint64_t *)a > *(uint64_t *)b) return 1;
    return 0;
}

static uint64_t average(uint64_t *t, size_t tlen) 
{
    size_t i;
    uint64_t acc=0;

    for(i=0;i<tlen;i++)
        acc += t[i];

    return acc/tlen;
}

void print_results(const char *s, uint64_t *t, size_t tlen) 
{
    size_t i;
    static uint64_t overhead = -1;

    if(tlen < 2) 
    {
        fprintf(stderr, "ERROR: Need a least two cycle counts!\n");
        return;
    }

    if(overhead  == (uint64_t)-1)
        overhead = cpucycles_overhead();

    tlen--;
    for(i=0;i<tlen;++i)
        t[i] = t[i+1] - t[i] - overhead;

    printf("%s\n", s);
    printf("average: %llu cycles\n", (unsigned long long)average(t, tlen));
    printf("\n");
}