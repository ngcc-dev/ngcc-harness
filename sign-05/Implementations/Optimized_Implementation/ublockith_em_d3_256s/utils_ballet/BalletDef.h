#ifndef _H_BalletCRYTYPE_
#define _H_BalletCRYTYPE_
#include <stdio.h>
#include <string.h> 
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#endif
#include <stdint.h>  

#define RoundBallet256256 74

#define Dirt_ENC 0
#define Dirt_DEC 1


#ifndef u8i
#define u8i unsigned char
#endif

#ifndef u16i
#define u16i uint16_t
#endif

#ifndef u32i
#define u32i uint32_t 
#endif

#ifndef u64i
#define u64i  uint64_t
#endif



/******************************
 swap and rotate
********************************/
#define ROTL4(v, n) \
  (((v) << (n)) | ((v) >> (4 - (n)))) 

#define ROTL8(v, n) \
  ((u8i)((v) << (n)) | ((v) >> (8 - (n)))) 

#define ROTL16(v, n) \
  ((u16i)((v) << (n)) | ((v) >> (16 - (n)))) 

#define ROTR16(v, n) \
  ((u16i)((v) >> (n)) | ((v) << (16 - (n))))   

#define SWAP16(v)\
 ROTL16(v, 8)

#define ROTL32(v, n) \
  ((u32i)((v) << (n)) | ((v) >> (32 - (n))))

#define ROTR32(v, n) \
  ((u32i)((v) >> (n)) | ((v) <<(32 - (n))))

#define SWAP32(v) \
  ((ROTL32(v,  8) & (u32i)(0x00FF00FF)) | \
   (ROTL32(v, 24) & (u32i)(0xFF00FF00))) 

#define SWAP64(v) \
  (((u64i)SWAP32((u32i)(v)) << 32) | (u64i)SWAP32((u32i)(v >> 32)))


#define ROTL64(v, n) \
  ((u64i)((v) << (n)) | ((v) >> (64 - (n))))

#define ROTR64(v, n) \
  ((u64i)((v) >> (n)) | ((v) << (64 - (n)))) 


#ifdef  __GNUC__
#define _C_ALIGN(n, A)  A __attribute__ ((aligned (n)))
#elif _MSC_VER 
#define _C_ALIGN(n, A)  __declspec( align(n) ) A
#endif

#define _C_ALIGN16(A) _C_ALIGN(16,A)
#define _C_ALIGN32(A) _C_ALIGN(32,A)
#endif 
