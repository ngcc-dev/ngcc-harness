/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"
#include <string.h>

static const u64 IV[IV_LANE_NUM] = {
0xC63471C8A9B1702A, 0x9131EE6B18889170, 
0x1701B3BAB89C5A08, 0x635EB7B10580D109, 
0x72153ED182F6EBFB, 0x96F93E8D3CAAE500, 
0xA98B10BDEE3BA4FA, 0x2DCD8C5F294403FA, 
0xFDD3B4364B999591, 0x14311D2F70498F2D, 
0x6332AC77291C10BF, 0x8FF1F9CEE145AED7, 
0xBC3EB70D5574E8D7, 0x24D1C3E7CF0DD2BA, 
0xB5A2FFC3B0EAE1C4, 0xC867C18B5C7C055F, 
0x669A8E1412CC3320};

#define _ra   1 
#define _rb  10
#define  _rc  24
#define  _rd  33
#define  _re   49

#define ROTL64(v, n) \
  ((uint64_t)((v) << (n)) | ((v) >> (64 - (n))))
  
/* --- 1. Permutation Declaration and Definition -*/

#define TOTAL_ROUNDS 16
 

#define  ARISPRD()\
s33=ROTL64(s33,_rd);\
    s11=s11+s44;s12=s12+s40;s13=s13+s41;s00^=iRC;\
s34=ROTL64(s34,_rd);s30=ROTL64(s30,_rd); iRC+=0x0f;\
s33=s33^s11;s34=s34^s12;s30=s30^s13; \
s22=ROTL64(s22,_rc); s23=ROTL64(s23,_rc);\
s00=s00+s33;s01=s01+s34;s02=s02+s30;\
s24=ROTL64(s24,_rc);\
s22=s22^s00;s23=s23^s01;s24=s24^s02;\
s11=ROTL64(s11,_rb);  s12=ROTL64(s12,_rb);\
s44=s44+s22;s40=s40+s23;s41=s41+s24;\
s13=ROTL64(s13,_rb);s00=ROTL64(s00,_ra);\
s11=s11^s44;s12=s12^s40;s13=s13^s41;\
s01=ROTL64(s01,_ra);s02=ROTL64(s02,_ra);\
s33=s33+s11;s34=s34+s12;s30=s30+s13; \
s44=ROTL64(s44,_re);s40=ROTL64(s40,_re);\
\
s00=s00^s33;s01=s01^s34;s02=s02^s30;\
s22=s22+s00;s23=s23+s01;s41=ROTL64(s41,_re); s24=s24+s02;\
s44=s44^s22;s40=s40^s23;s41=s41^s24;\
s31=ROTL64(s31,_rd);s32=ROTL64(s32,_rd);\
s14=s14+s42;s10=s10+s43;\
s20=ROTL64(s20,_rc);s21=ROTL64(s21,_rc);\
s31=s31^s14;s32=s32^s10;\
s03=s03+s31;s04=s04+s32;\
s14=ROTL64(s14,_rb);s10=ROTL64(s10,_rb);\
s20=s20^s03;s21=s21^s04;\
s42=s42+s20;s43=s43+s21;\
s03=ROTL64(s03,_ra);s04=ROTL64(s04,_ra);\
s14=s14^s42;s10=s10^s43;\
s31=s31+s14;s32=s32+s10;\
s03=s03^s31;s04=s04^s32;\
s42=ROTL64(s42,_re);s43=ROTL64(s43,_re);\
s20=s20+s03;s21=s21+s04; s01=s01+s04;s11=s11+s14;\
s03=ROTL64(s03,_rd);s13=ROTL64(s13,_rd);\
s42=s42^s20;s43=s43^s21; \
s03=s03^s01;s13=s13^s11;\
s02=ROTL64(s02,_rc);s12=ROTL64(s12,_rc);\
\
s00=s00+s03;s10=s10+s13;\
s01=ROTL64(s01,_rb);s11=ROTL64(s11,_rb);\
s02=s02^s00;s12=s12^s10;\
s00=ROTL64(s00,_ra);s10=ROTL64(s10,_ra);\
s04=s04+s02;s14=s14+s12;\
s01=s01^s04;s11=s11^s14;\
s03=s03+s01;s13=s13+s11;\
s00=s00^s03;s10=s10^s13;\
s02=s02+s00;s12=s12+s10; \
s04=ROTL64(s04,_re);s14=ROTL64(s14,_re);\
s04=s04^s02;s14=s14^s12;\
s21=s21+s24;s31=s31+s34;s41=s41+s44;\
s23=ROTL64(s23,_rd);s33=ROTL64(s33,_rd);s43=ROTL64(s43,_rd);\
s23=s23^s21;s33=s33^s31;s43=s43^s41;\
s20=s20+s23;s30=s30+s33;s40=s40+s43;\
s22=ROTL64(s22,_rc);s32=ROTL64(s32,_rc);s42=ROTL64(s42,_rc);\
s22=s22^s20;s32=s32^s30;s42=s42^s40;\
s24=s24+s22;s34=s34+s32;s44=s44+s42;\
s21=ROTL64(s21,_rb);s31=ROTL64(s31,_rb);s41=ROTL64(s41,_rb);\
s21=s21^s24;s31=s31^s34;s41=s41^s44;\
s23=s23+s21;s33=s33+s31;\
s20=ROTL64(s20,_ra);s30=ROTL64(s30,_ra);s40=ROTL64(s40,_ra);\
s20=s20^s23;s30=s30^s33;s43=s43+s41;s40=s40^s43;\
s22=s22+s20;s32=s32+s30;\
s24=ROTL64(s24,_re);\
s24=s24^s22;s42=s42+s40;\
s34=ROTL64(s34,_re);s44=ROTL64(s44,_re);\
s34=s34^s32;s44=s44^s42;  
  
 
#define ROTL32(v, n) \
  ((uint32_t)((v) << (n)) | ((v) >> (32 - (n))))
 

#define SWAP32(v) \
  ((ROTL32(v,  8) & (uint32_t)(0x00FF00FF)) | \
   (ROTL32(v, 24) & (uint32_t)(0xFF00FF00)))

#define SWAP64(v) \
  (((uint64_t)SWAP32((uint32_t)(v)) << 32) | (uint64_t)SWAP32((uint32_t)(v >> 32)))

 

int CryptHash( int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[18]; //9
    
    /* --- 1. Initialize Phase --- */
//    u64 X[5][5] = {0};  
	uint64_t s00,s01,s02,s03,s04;
	uint64_t s10,s11,s12,s13,s14;
	uint64_t s20,s21,s22,s23,s24;
	uint64_t s30,s31,s32,s33,s34;
	uint64_t s40,s41,s42,s43,s44; 
   uint64_t iRC=0x0f;
	
 
  	s00=IV[16];  s01=IV[11];	s02=IV[6];	s03=IV[1];	s04=0;
	s10=IV[15];  s11=IV[10];	s12=IV[5];	s13=IV[0];	s14=0;
	s20=IV[14];  s21=IV[9];	s22=IV[4];	s23=0;	s24=0;
	s30=IV[13];  s31=IV[8];	s32=IV[3];	s33=0;	s34=0;
	s40=IV[12];  s41=IV[7];	s42=IV[2];	s43=0;	s44=0;  
 
    /* --- 2. Absorbing Phase --- */
   
    while (rem_bits >= RATE_BITS) {
   
        S[16]=s00; S[15]=s10;S[14]=s20; S[13]=s30; S[12]=s40;
        S[11]=s01; S[10]=s11; S[9]=s21; S[8]=s31;  S[7]=s41;
        S[6]=s02; S[5]=s12; S[4]=s22;  S[3]=s32; S[2]=s42;
		S[1]=s03; S[0]= s13; 
        
        s44^=SWAP64((*(uint64_t * )(msg )));
        s34^=SWAP64((*(uint64_t * )(msg +8)));
        s24^=SWAP64((*(uint64_t * )(msg +16)));
        s14^=SWAP64((*(uint64_t * )(msg +24)));
        s04^=SWAP64((*(uint64_t * )(msg +32)));
        s43^=SWAP64((*(uint64_t * )(msg +40)));
        s33^=SWAP64((*(uint64_t * )(msg +48)));
        s23^=SWAP64((*(uint64_t * )(msg +56)));
       for(int i=0;i<TOTAL_ROUNDS;i++)
	   {
         ARISPRD();
	   }
	   iRC=0x0f;
	       
  	s00^=S[16]; 	s01^=S[11];  s02^=S[6];  s03^=S[1];	 
	s10^=S[15];   s11^=S[10];  s12^=S[5];    s13^=S[0];
	s20^=S[14];  s21^=S[9]; s22^=S[4];
	s30^=S[13];  s31^=S[8]; s32^=S[3];
	s40^=S[12];  s41^=S[7]; s42^=S[2]; 

        msg += RATE_BYTES;
        rem_bits -= RATE_BITS;
    }

    int need_extra_zero_block = (rem_bits == RATE_BITS - 1ULL);
    /* --- 3. Padding pd10* --- */
    unsigned char pad_block[RATE_BYTES] = {0};
    unsigned int full_bytes = (unsigned int)(rem_bits / 8);
    unsigned int partial_bits = (unsigned int)(rem_bits % 8);
    
    if (full_bytes > 0) {
        memcpy(pad_block, msg, full_bytes);
    }

    if (partial_bits > 0) {
        pad_block[full_bytes] = msg[full_bytes] & (0xFFU << (8 - partial_bits));
        pad_block[full_bytes] |= (0x80U >> partial_bits);
    } else {
        pad_block[full_bytes] = 0x80U;
    }

    
        s44^=SWAP64((*(uint64_t * )(pad_block )));
        s34^=SWAP64((*(uint64_t * )(pad_block +8)));
        s24^=SWAP64((*(uint64_t * )(pad_block +16)));
        s14^=SWAP64((*(uint64_t * )(pad_block +24)));
        s04^=SWAP64((*(uint64_t * )(pad_block +32)));
        s43^=SWAP64((*(uint64_t * )(pad_block +40)));
        s33^=SWAP64((*(uint64_t * )(pad_block +48)));
        s23^=SWAP64((*(uint64_t * )(pad_block +56)));
        s00^=!need_extra_zero_block;
        
        S[16]=s00; S[15]=s10;S[14]=s20; S[13]=s30; S[12]=s40;
        S[11]=s01; S[10]=s11; S[9]=s21; S[8]=s31;  S[7]=s41;
        S[6]=s02; S[5]=s12; S[4]=s22;  S[3]=s32; S[2]=s42;
		S[1]=s03; S[0]= s13; 
        
       for(int i=0;i<TOTAL_ROUNDS;i++)
	   {
         ARISPRD();
	   }

  	s00^=S[16]; 	s01^=S[11];  s02^=S[6];  s03^=S[1];	 
	s10^=S[15];   s11^=S[10];  s12^=S[5];    s13^=S[0];
	s20^=S[14];  s21^=S[9]; s22^=S[4];
	s30^=S[13];  s31^=S[8]; s32^=S[3];
	s40^=S[12];  s41^=S[7]; s42^=S[2]; 
if(need_extra_zero_block)
{
          s00^=1;
	   iRC=0x0f;
        
        S[16]=s00; S[15]=s10;S[14]=s20; S[13]=s30; S[12]=s40;
        S[11]=s01; S[10]=s11; S[9]=s21; S[8]=s31;  S[7]=s41;
        S[6]=s02; S[5]=s12; S[4]=s22;  S[3]=s32; S[2]=s42;
		S[1]=s03; S[0]= s13; 
        
       for(int i=0;i<TOTAL_ROUNDS;i++)
	   {
         ARISPRD();
	   }

  	s00^=S[16]; 	s01^=S[11];  s02^=S[6];  s03^=S[1];	 
	s10^=S[15];   s11^=S[10];  s12^=S[5];    s13^=S[0];
	s20^=S[14];  s21^=S[9]; s22^=S[4];
	s30^=S[13];  s31^=S[8]; s32^=S[3];
	s40^=S[12];  s41^=S[7]; s42^=S[2]; 
}
    /* --- 5. Squeezing Phase --- */
    *((uint64_t *)(digest ))= SWAP64(s03);
    *((uint64_t *)(digest +8 ))= SWAP64(s42);
    *((uint64_t *)(digest +16))= SWAP64(s32);
    *((uint64_t *)(digest +24))= SWAP64(s22);
    *((uint64_t *)(digest +32))= SWAP64(s12);
    *((uint64_t *)(digest +40 ))= SWAP64(s02);
    *((uint64_t *)(digest +48))= SWAP64(s41);
    *((uint64_t *)(digest +56))= SWAP64(s31);
    *((uint64_t *)(digest +64))= SWAP64(s21);
    *((uint64_t *)(digest+72 ))= SWAP64(s11);
    *((uint64_t *)(digest+80 ))= SWAP64(s01);
    *((uint64_t *)(digest+88 ))= SWAP64(s40);
    *((uint64_t *)(digest+96 ))= SWAP64(s30);
    *((uint64_t *)(digest+104 ))= SWAP64(s20);
    *((uint64_t *)(digest+112 ))= SWAP64(s10);
    *((uint64_t *)(digest+120))= SWAP64(s00);

    return 0;
}
