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
static const uint64_t RC[12] __attribute__ ((aligned (32)))= {
    0x0001ULL, 0x0003ULL, 0x000AULL, 0x0088ULL, 
    0x8081ULL, 0x8002ULL, 0x0009ULL, 0x0083ULL, 
    0x800AULL, 0x0089ULL, 0x8083ULL, 0x800BULL
};
static const u64 IV[IV_LANE_NUM] = {
    0xE1F7C9010E8EC178ULL,
    0x676B3791EBA6281CULL,
    0xA7A16EAC10689E8CULL,
    0x9CCCEF7ACDC19758ULL,
    0x900E93CA9A4A9953ULL,
    0x55FBD3148A3A352AULL,
    0xDCFA816272A35C8CULL,
    0xB9839A4AF0CAE3D5ULL,
    0xDB34E00BB8122876ULL,
    0xCC3FA3279D6371F6ULL,
    0x1E6D7FF8F050881CULL,
    0x9C67610B0EF5484CULL,
    0x1EE2C8609716E1FDULL
};
 

#define ROTL64(v, n) \
  ((uint64_t)((v) << (n)) | ((v) >> (64 - (n))))
  
#define ROTR64(v, n) \
  ((uint64_t)((v) >> (n)) | ((v) << (64 - (n))))


#define ROTL64(v, n) \
  ((uint64_t)((v) << (n)) | ((v) >> (64 - (n))))
  
#define ROTR64(v, n) \
  ((uint64_t)((v) >> (n)) | ((v) << (64 - (n))))
  
  
#define ROTL32(v, n) \
  ((uint32_t)((v) << (n)) | ((v) >> (32 - (n))))
 
#define SWAP32(v) \
  ((ROTL32(v,  8) & (uint32_t)(0x00FF00FF)) | \
   (ROTL32(v, 24) & (uint32_t)(0xFF00FF00)))

#define SWAP64(v) \
  (((uint64_t)SWAP32((uint32_t)(v)) << 32) | (uint64_t)SWAP32((uint32_t)(v >> 32)))
  
  
#define ANDN(a,b) ((~(a))&(b))
#define OR(a,b)  (a|b)
#define Chi(oa,ob,oc,od,oe,a,b,c,d,e) \
{  \
oa=d^ANDN( e, ANDN(b,c) );\
ob=e^ANDN( a, ANDN(c,d) );\
oc=a^ANDN( b, ANDN(d,e) );\
od=b^ANDN( c, ANDN(e,a) );\
oe=c^ANDN( d, ANDN(a,b) );\
}

 //295 steps 
#define Chisub(a,b,c,d)  a^ANDN(b, ANDN(c,d)))

#define Thunder_12rTmacro(o, s,r )\
	s##00^=RC[r];\
s##40=ROTR64(s##40,6);\
s##30=ROTR64(s##30,48);\
	q0= s##30^ROTR64(s##40,17);\
	p0=s##30^s##40;\
s##20=ROTR64(s##20,34);\
	q0= s##20^ROTR64(q0,4); \
	p0=p0^s##20;\
s##10=ROTR64(s##10,40);\
	q0=s##10^ROTR64(q0,20); \
	p0=p0^s##10;\
s##00=ROTR64(s##00,45);\
	q0=s##00^ROTR64(q0,9);\
	p0=p0^s##00;\
	\
s##41=ROTR64(s##41,18);\
s##31=ROTR64(s##31,30);\
	q1= s##31^ROTR64(s##41,17);\
	p1=s##31^s##41; \
s##21=ROTR64(s##21,5);\
	q1= s##21^ROTR64(q1,4); \
	p1=p1^s##21;\
s##11=ROTR64(s##11,52);\
	q1=s##11^ROTR64(q1,20); \
	p1=p1^s##11;\
s##01=ROTR64(s##01,33); \
	q1=s##01^ROTR64(q1,9); \
	p1=p1^s##01;\
\
s##42=ROTR64(s##42,49);\
s##32=ROTR64(s##32,61);\
	q2= s##32^ROTR64(s##42,17);\
	p2=s##32^s##42;\
	q2= s##22^ROTR64(q2,4);  \
	p2^=s##22;\
s##12=ROTR64(s##12,19);\
	q2=s##12^ROTR64(q2,20); \
	p2^=s##12;\
s##02=ROTR64(s##02,13);\
	q2=s##02^ROTR64(q2,9); \
	p2^=s##02;\
\
s##43=ROTR64(s##43,17);\
s##33=ROTR64(s##33,8);\
	q3= s##33^ROTR64(s##43,17); \
	p3=s##33^s##43;\
s##23=ROTR64(s##23,2);\
	q3= s##23^ROTR64(q3,4); \
	p3^=s##23;\
s##13=ROTR64(s##13,47);\
	q3=s##13^ROTR64(q3,20); \
	p3^=s##13;\
s##03=ROTR64(s##03,1);\
	q3=s##03^ROTR64(q3,9); \
	p3^=s##03;\
\
s##44=ROTR64(s##44,43);\
s##34=ROTR64(s##34,38);\
	q4= s##34^ROTR64(s##44,17);\
	p4=s##34^s##44;\
s##24=ROTR64(s##24,37);\
	q4= s##24^ROTR64(q4,4); \
	p4^=s##24;\
s##14=ROTR64(s##14,9);\
	q4=s##14^ROTR64(q4,20); \
	p4^=s##14;\
s##04=ROTR64(s##04,27); \
	q4=s##04^ROTR64(q4,9);  \
	p4^=s##04;\
	\
	e0=ROTR64(p0^ROTR64(p4,1),26);  \
	f0=ROTR64(q3,36)^q1;\
	e2=ROTR64(p2^ROTR64(p1,1),26);\
	f2=ROTR64(q0,36)^q3;\
	e3=ROTR64(p3^ROTR64(p2,1),26);\
	f3=ROTR64(q1,36)^q4;\
	e1=ROTR64(p1^ROTR64(p0,1),26);\
	f1=ROTR64(q4,36)^q2;\
	e4=ROTR64(p4^ROTR64(p3,1),26); \
	f4=ROTR64(q2,36)^q0; \
	\
	s##00^=e0^ROTR64(f0,44);\
	s##02^=e2^ROTR64(f2,44);\
	 o##00=ANDN(s##02,s##00);\
	s##03^=e3^ROTR64(f3,44);\
	 o##10=ANDN(s##00,s##03);\
	s##01^=e1^ROTR64(f1,44);\
	o##20=ANDN(s##03,s##01);\
	s##04^=e4^ROTR64(f4,44);\
 o##30=OR(s##00,s##01) ; o##40=OR(s##03, s##04);\
 o##00=s##03^ANDN( s##01,  o##00 );\
 o##10=s##01^ANDN( s##04, o##10  );\
 o##20=s##04^ANDN( s##02,o##20  );\
 o##30=s##02^ANDN( o##30 ,s##04);\
 o##40=s##00^ANDN( o##40 ,s##02 );\
\
	s##11^=e1^ROTR64(f1,53);\
	s##13^=e3^ROTR64(f3,53);\
	o##01=ANDN(s##13,s##11);\
	s##14^=e4^ROTR64(f4,53);\
	o##11=ANDN(s##11,s##14);\
	s##12^=e2^ROTR64(f2,53);\
	o##21=ANDN(s##14,s##12);\
	s##10^=e0^ROTR64(f0,53);\
o##31= OR(s##11,s##12); o##41=OR(s##14,s##10); \
o##01=s##14^ANDN( s##12,o##01  );\
o##11=s##12^ANDN( s##10, o##11 );\
o##21=s##10^ANDN( s##13,o##21  );\
o##31=s##13^ANDN( o##31,s##10  );\
o##41=s##11^ANDN( o##41 ,s##13 );\
\
	s##24^=e4^ROTR64(f4,9);\
	s##22^=e2^ROTR64(f2,9);\
	o##02=ANDN(s##24,s##22);\
	s##20^=e0^ROTR64(f0,9);\
	o##12=ANDN(s##22,s##20);\
	s##23^=e3^ROTR64(f3,9);\
	o##22=ANDN(s##20,s##23);\
	s##21^=e1^ROTR64(f1,9);\
o##32=OR(s##22, s##23);o##42=OR(s##20, s##21);\
o##02=s##20^ANDN( s##23, o##02 );\
o##12=s##23^ANDN( s##21, o##12 );\
o##22=s##21^ANDN( s##24, o##22 );\
o##32=s##24^ANDN( o##32 ,s##21);\
o##42=s##22^ANDN( o##42,s##24 );\
\
	s##33^=e3^ROTR64(f3,13);\
	s##30^=e0^ROTR64(f0,13);\
	o##03= ANDN(s##30,s##33);\
	s##31^=e1^ROTR64(f1,13);\
	o##13=ANDN(s##33,s##31) ;\
	s##34^=e4^ROTR64(f4,13);\
	o##23=OR(s##30,s##31);\
	s##32^=e2^ROTR64(f2,13);\
o##33=	OR(s##33,s##34) ;o##43=OR(s##31,s##32) ;\
o##03=s##31^ANDN( s##34,o##03 );\
o##13=s##34^ANDN( s##32,o##13 );\
o##23=s##32^ANDN(  o##23 ,s##34);\
o##33=s##30^ANDN( o##33 ,s##32);\
o##43=s##33^ANDN( o##43 ,s##30);\
\
	s##41^=e1^ROTR64(f1,30);\
	s##44^=e4^ROTR64(f4,30);\
	o##04=ANDN(s##41,s##44);\
	s##42^=e2^ROTR64(f2,30);\
	o##14=ANDN(s##44,s##42);\
	s##40^=e0^ROTR64(f0,30);\
	o##04=ANDN( s##40,o##04  );\
	o##24=OR(s##41, s##42);\
	s##43^=e3^ROTR64(f3,30);\
	o##24=ANDN( o##24,s##40 );\
	o##34=OR(s##40,s##44 ) ;\
	o##14=ANDN( s##43, o##14 );\
	o##44=OR(s##42,s##43) ;\
o##34=ANDN( o##34,s##43 );\
o##44=ANDN( o##44,s##41 );\
o##04=s##42^o##04;\
o##14=s##40^o##14;\
o##24=s##43^o##24;\
o##34=s##41^o##34;\
o##44=s##44^o##44;
 

#define TOTAL_ROUNDS 12
 
int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[16]; //9
    
	uint64_t s00,s01,s02,s03,s04;
	uint64_t s10,s11,s12,s13,s14;
	uint64_t s20,s21,s22,s23,s24;
	uint64_t s30,s31,s32,s33,s34;
	uint64_t s40,s41,s42,s43,s44;
	uint64_t o00,o01,o02,o03,o04;
	uint64_t o10,o11,o12,o13,o14;
	uint64_t o20,o21,o22,o23,o24;
	uint64_t o30,o31,o32,o33,o34;
	uint64_t o40,o41,o42,o43,o44;
	
	uint64_t p0,p1,p2,p3,p4;
	uint64_t q0,q1,q2,q3,q4;
	uint64_t e0,e1,e2,e3,e4;
	uint64_t f0,f1,f2,f3,f4;
	uint64_t t0,t1,t2,t3,t4; 
    /* --- 1. Initialize Phase --- */ 
 

  	s00=IV[12]; 	s10=IV[7];	s20=IV[2];	s30=0;	s40=0;
	s01=IV[11];   s11=IV[6];	s21=IV[1];	s31=0;	s41=0;
	s02=IV[10];  s12=IV[5];	s22=IV[0];	s32=0;	s42=0;
	s03=IV[9];  s13=IV[4];	s23=0;	s33=0;	s43=0;
	s04=IV[8];  s14=IV[3];	s24=0;	s34=0;	s44=0;  
 
    /* --- 2. Absorbing Phase --- */
   
    while (rem_bits >= RATE_BITS) {
    	 
 S[12]=s00; S[11]=s01;S[10]=s02; S[9]=s03; S[8]=s04;
        S[7]=s10; S[6]=s11; S[5]=s12; S[4]=s13;  S[3]=s14;
        S[2]=s20; S[1]=s21; S[0]=s22;
        s44^=SWAP64((*(uint64_t * )(msg )));
        s43^=SWAP64((*(uint64_t * )(msg +8)));
        s42^=SWAP64((*(uint64_t * )(msg +16)));
        s41^=SWAP64((*(uint64_t * )(msg +24)));
        s40^=SWAP64((*(uint64_t * )(msg +32)));
        s34^=SWAP64((*(uint64_t * )(msg +40)));
        s33^=SWAP64((*(uint64_t * )(msg +48)));
        s32^=SWAP64((*(uint64_t * )(msg +56)));
        s31^=SWAP64((*(uint64_t * )(msg +64)));
        s30^=SWAP64((*(uint64_t * )(msg +72)));
        s24^=SWAP64((*(uint64_t * )(msg +80)));
        s23^=SWAP64((*(uint64_t * )(msg +88)));
       for(int i=0;i<TOTAL_ROUNDS;i+=2)
	   {
	      Thunder_12rTmacro(o,s,i);
	      Thunder_12rTmacro(s,o,i+1);
	   } 
	       
	s00^=S[12]; s10^=S[7];  s20^=S[2];	 
	s01^=S[11]; s11^=S[6];  s21^=S[1];
	s02^=S[10]; s12^=S[5]; s22^=S[0];
	s03^=S[9];  s13^=S[4]; 
	s04^=S[8];  s14^=S[3];  
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
        s43^=SWAP64((*(uint64_t * )(pad_block +8)));
        s42^=SWAP64((*(uint64_t * )(pad_block +16)));
        s41^=SWAP64((*(uint64_t * )(pad_block +24)));
        s40^=SWAP64((*(uint64_t * )(pad_block +32)));
        s34^=SWAP64((*(uint64_t * )(pad_block +40)));
        s33^=SWAP64((*(uint64_t * )(pad_block +48)));
        s32^=SWAP64((*(uint64_t * )(pad_block +56)));
        s31^=SWAP64((*(uint64_t * )(pad_block +64)));
        s30^=SWAP64((*(uint64_t * )(pad_block +72)));
        s24^=SWAP64((*(uint64_t * )(pad_block +80)));
        s23^=SWAP64((*(uint64_t * )(pad_block +88)));
        s00^=!need_extra_zero_block;
        S[12]=s00; S[11]=s01;S[10]=s02; S[9]=s03; S[8]=s04;
        S[7]=s10; S[6]=s11; S[5]=s12; S[4]=s13;  S[3]=s14;
        S[2]=s20; S[1]=s21; S[0]=s22;
        
       for(int i=0;i<TOTAL_ROUNDS;i+=2)
	   {
	      Thunder_12rTmacro(o,s,i);
	      Thunder_12rTmacro(s,o,i+1);
	   }

  	s00^=S[12]; s10^=S[7];  s20^=S[2];	 
	s01^=S[11]; s11^=S[6];  s21^=S[1];
	s02^=S[10]; s12^=S[5]; s22^=S[0];
	s03^=S[9];  s13^=S[4]; 
	s04^=S[8];  s14^=S[3]; 
	if(need_extra_zero_block)
	{
	  s00^= 1;
        S[12]=s00; S[11]=s01;S[10]=s02; S[9]=s03; S[8]=s04;
        S[7]=s10; S[6]=s11; S[5]=s12; S[4]=s13;  S[3]=s14;
        S[2]=s20; S[1]=s21;// S[0]=s22;
        
       for(int i=0;i<TOTAL_ROUNDS;i+=2)
	   {
	      Thunder_12rTmacro(o,s,i);
	      Thunder_12rTmacro(s,o,i+1);
	   }

  	s00^=S[12]; s10^=S[7];  s20^=S[2];	 
	s01^=S[11]; s11^=S[6];  s21^=S[1];
	s02^=S[10]; s12^=S[5]; //s22^=S[0];
	s03^=S[9];  s13^=S[4]; 
	s04^=S[8];  s14^=S[3]; 
	}
	
    /* --- 5. Squeezing Phase --- */
    *((uint64_t *)(digest ))= SWAP64(s21);
    *((uint64_t *)(digest +8 ))= SWAP64(s20);
    *((uint64_t *)(digest +16))= SWAP64(s14);
    *((uint64_t *)(digest +24))= SWAP64(s13);
    *((uint64_t *)(digest +32))= SWAP64(s12);
    *((uint64_t *)(digest+40 ))= SWAP64(s11);
    *((uint64_t *)(digest+48 ))= SWAP64(s10);
    *((uint64_t *)(digest+56 ))= SWAP64(s04);
    *((uint64_t *)(digest+64 ))= SWAP64(s03);
    *((uint64_t *)(digest+72 ))= SWAP64(s02);
    *((uint64_t *)(digest+80 ))= SWAP64(s01);
    *((uint64_t *)(digest+88 ))= SWAP64(s00);   

    return 0;
} 
