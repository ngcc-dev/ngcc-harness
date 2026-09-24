/*
 * test_hash.c - SM3 KAT + Hw width + salted TCR commitment
 */
#include "hash.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

int main(void){
    /* 1) SM3 standard KAT: SM3("abc") */
    const uint8_t exp_abc[32]={
      0x66,0xc7,0xf0,0xf4,0x62,0xee,0xed,0xd9,0xd1,0xf2,0xd4,0x6b,0xdc,0x10,0xe4,0xe2,
      0x41,0x67,0xc4,0x87,0x5c,0xf2,0xf7,0xa2,0x29,0x7d,0xa0,0x2b,0x8f,0x4b,0xa8,0xe0};
    uint8_t d[32]; sm3((const uint8_t*)"abc",3,d);
    assert(memcmp(d,exp_abc,32)==0);

    /* 2) Hw output width = PARAM_HASH_BYTES */
    uint8_t out[PARAM_HASH_BYTES];
    hash_digest(out,(const uint8_t*)"x",1);

    /* 3) Salted commitment: different salt -> different; same salt+input -> same */
    uint8_t salt1[16], salt2[16];
    memset(salt1,1,16); memset(salt2,2,16);
    uint8_t msg[8]; memset(msg,9,8);
    uint8_t c1[PARAM_HASH_BYTES],c2[PARAM_HASH_BYTES],c3[PARAM_HASH_BYTES];
    hash_commit(c1, salt1,16, 0, msg,8);
    hash_commit(c2, salt2,16, 0, msg,8);
    hash_commit(c3, salt1,16, 0, msg,8);
    assert(memcmp(c1,c2,PARAM_HASH_BYTES)!=0);
    assert(memcmp(c1,c3,PARAM_HASH_BYTES)==0);

    /* 4) idx is bound: same salt+payload, different idx -> different commitment */
    uint8_t c4[PARAM_HASH_BYTES];
    hash_commit(c4, salt1,16, 1, msg,8);
    assert(memcmp(c1,c4,PARAM_HASH_BYTES)!=0);

    printf("test_hash OK\n");
    return 0;
}
