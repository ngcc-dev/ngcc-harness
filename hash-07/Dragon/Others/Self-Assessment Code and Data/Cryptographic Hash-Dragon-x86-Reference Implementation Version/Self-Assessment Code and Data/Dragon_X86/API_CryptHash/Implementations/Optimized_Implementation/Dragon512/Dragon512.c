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
0x2D5EBD181E489EBB, 
0xDDAA68D459FF4EE7, 
0xD8F5D940D993F0D3, 
0x86F2A0CA3B8E0D1E, 
0xDC1851BCA4509C55, 
0x8760145EFF9D8EB9, 
0x9B39251883C81294, 
0x4402CE26DD9509A5, 
0x0B6B2E774D8F675B };

#define _ra   1 
#define _rb  10
#define  _rc  24
#define  _rd  33
#define  _re   49

#define ROTL64(v, n) \
  ((uint64_t)((v) << (n)) | ((v) >> (64 - (n))))
  
  
   
#define ROTL32(v, n) \
  ((uint32_t)((v) << (n)) | ((v) >> (32 - (n))))
 

#define SWAP32(v) \
  ((ROTL32(v,  8) & (uint32_t)(0x00FF00FF)) | \
   (ROTL32(v, 24) & (uint32_t)(0xFF00FF00)))

#define SWAP64(v) \
  (((uint64_t)SWAP32((uint32_t)(v)) << 32) | (uint64_t)SWAP32((uint32_t)(v >> 32)))


  
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


int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[IV_LANE_NUM+7]; //9
    
	uint64_t s00,s01,s02,s03,s04;
	uint64_t s10,s11,s12,s13,s14;
	uint64_t s20,s21,s22,s23,s24;
	uint64_t s30,s31,s32,s33,s34;
	uint64_t s40,s41,s42,s43,s44; 
   uint64_t iRC=0x0f;
	
    /* --- 1. Initialize Phase --- */ 
 
  	s00=IV[8]; 	s01=IV[3];	s02=0;	s03=0;	s04=0;
	s10=IV[7];   s11=IV[2];	s12=0;	s13=0;	s14=0;
	s20=IV[6];  s21=IV[1];	s22=0;	s23=0;	s24=0;
	s30=IV[5];  s31=IV[0];	s32=0;	s33=0;	s34=0;
	s40=IV[4];  s41=0;	s42=0;	s43=0;	s44=0;  

    /* --- 2. Absorbing Phase --- */
    while (rem_bits >= RATE_BITS) {
    	 
        S[8]=s00; S[7]=s10;S[6]=s20; S[5]=s30; S[4]=s40;
        S[3]=s01; S[2]=s11; S[1]=s21; S[0]=s31;
        
        s44^=SWAP64((*(uint64_t * )(msg )));
        s34^=SWAP64((*(uint64_t * )(msg +8)));
        s24^=SWAP64((*(uint64_t * )(msg +16)));
        s14^=SWAP64((*(uint64_t * )(msg +24)));
        s04^=SWAP64((*(uint64_t * )(msg +32)));
        s43^=SWAP64((*(uint64_t * )(msg +40)));
        s33^=SWAP64((*(uint64_t * )(msg +48)));
        s23^=SWAP64((*(uint64_t * )(msg +56)));
        s13^=SWAP64((*(uint64_t * )(msg +64)));
        s03^=SWAP64((*(uint64_t * )(msg +72)));
        s42^=SWAP64((*(uint64_t * )(msg +80)));
        s32^=SWAP64((*(uint64_t * )(msg +88)));
        s22^=SWAP64((*(uint64_t * )(msg +96)));
        s12^=SWAP64((*(uint64_t * )(msg +104)));
        s02^=SWAP64((*(uint64_t * )(msg +112)));
        s41^=SWAP64((*(uint64_t * )(msg +120)));
       for(int i=0;i<TOTAL_ROUNDS;i++)
	   {
         ARISPRD();
	   }
	   iRC=0x0f;
	       
  	s00^=S[8]; 	s01^=S[3];	 
	s10^=S[7];   s11^=S[2]; 
	s20^=S[6];  s21^=S[1]; 
	s30^=S[5];  s31^=S[0]; 
	s40^=S[4];  

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
        s13^=SWAP64((*(uint64_t * )(pad_block +64)));
        s03^=SWAP64((*(uint64_t * )(pad_block +72)));
        s42^=SWAP64((*(uint64_t * )(pad_block +80)));
        s32^=SWAP64((*(uint64_t * )(pad_block +88)));
        s22^=SWAP64((*(uint64_t * )(pad_block +96)));
        s12^=SWAP64((*(uint64_t * )(pad_block +104)));
        s02^=SWAP64((*(uint64_t * )(pad_block +112)));
        s41^=SWAP64((*(uint64_t * )(pad_block +120)));
        s00^=!need_extra_zero_block;
        S[8]=s00; S[7]=s10;S[6]=s20; S[5]=s30; S[4]=s40;
        S[3]=s01; S[2]=s11; S[1]=s21; S[0]=s31;
       for(int i=0;i<TOTAL_ROUNDS;i++)
	   {
         ARISPRD();
	   }
	s00^=S[8]; 	s01^=S[3];	 
	s10^=S[7];   s11^=S[2]; 
	s20^=S[6];  s21^=S[1]; 
	s30^=S[5];  s31^=S[0]; 
	s40^=S[4];
	
    if( need_extra_zero_block)
    {
         s00^=1;
	 iRC=0x0f;
        S[8]=s00; S[7]=s10;S[6]=s20; S[5]=s30; S[4]=s40;
        S[3]=s01; S[2]=s11; S[1]=s21; S[0]=s31;
       for(int i=0;i<TOTAL_ROUNDS;i++)
	 {
         ARISPRD();
	}
	s00^=S[8]; 	s01^=S[3];	 
	s10^=S[7];   s11^=S[2]; 
	s20^=S[6];  s21^=S[1]; 
	s30^=S[5];  s31^=S[0]; 
	s40^=S[4];
    
    }	
	
    /* --- 5. Squeezing Phase --- */
    *((uint64_t *)(digest ))= SWAP64(s21);
    *((uint64_t *)(digest+8 ))= SWAP64(s11);
    *((uint64_t *)(digest+16 ))= SWAP64(s01);
    *((uint64_t *)(digest+24))= SWAP64(s40);
    *((uint64_t *)(digest+32 ))= SWAP64(s30);
    *((uint64_t *)(digest+40 ))= SWAP64(s20);
    *((uint64_t *)(digest+48 ))= SWAP64(s10);
    *((uint64_t *)(digest+56 ))= SWAP64(s00);
    
    return 0;
}




